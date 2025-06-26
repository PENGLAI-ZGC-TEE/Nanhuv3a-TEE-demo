#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

#define PORT 8443
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Certificate and key file paths
#define SERVER_CERT "certs/server-cert.pem"
#define SERVER_KEY "certs/server-key.pem"
#define CA_CERT "certs/ca-cert.pem"

// Global variables
static WOLFSSL_CTX* g_ctx = NULL;
static int g_server_running = 1;
static int g_client_count = 0;
static pthread_mutex_t g_client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Data generation variables
static double g_data1 = 0.0;  // 50000-70000 range
static double g_data2 = 0.0;  // 800-1200 range
static pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_data_thread;

// Client connection structure
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    int client_id;
    WOLFSSL* ssl;
} client_info_t;

// Client list for broadcasting
static client_info_t* g_clients[MAX_CLIENTS];
static pthread_mutex_t g_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function declarations
void broadcast_data_to_clients(double data1, double data2);
void* data_generator(void* arg);
void* handle_client(void* arg);
void signal_handler(int sig);
double generate_normal(double mean, double stddev);

// Generate normal distribution random number using Box-Muller transform
double generate_normal(double mean, double stddev) {
    static int has_spare = 0;
    static double spare;
    
    if (has_spare) {
        has_spare = 0;
        return spare * stddev + mean;
    }
    
    has_spare = 1;
    static double u, v, mag;
    do {
        u = (double)rand() / RAND_MAX * 2.0 - 1.0;
        v = (double)rand() / RAND_MAX * 2.0 - 1.0;
        mag = u * u + v * v;
    } while (mag >= 1.0 || mag == 0.0);
    
    mag = sqrt(-2.0 * log(mag) / mag);
    spare = v * mag;
    return u * mag * stddev + mean;
}

// Data generation thread function
void* data_generator(void* arg) {
    (void)arg; // Suppress unused parameter warning
    
    while (g_server_running) {
        // Generate two normal distribution random numbers
        double data1 = generate_normal(61000.0, 1000.0);  // Mean=60000, range roughly 50000-70000
        double data2 = generate_normal(1000.0, 80.0);    // Mean=1000, range roughly 800-1200
        
        // Clamp values to desired ranges
        if (data1 < 50000.0) data1 = 50000.0;
        if (data1 > 70000.0) data1 = 70000.0;
        if (data2 < 800.0) data2 = 800.0;
        if (data2 > 1200.0) data2 = 1200.0;
        
        // Update global data with mutex protection
        pthread_mutex_lock(&g_data_mutex);
        g_data1 = data1;
        g_data2 = data2;
        pthread_mutex_unlock(&g_data_mutex);
        
        // Broadcast data to all connected clients
        broadcast_data_to_clients(data1, data2);
        
        printf("Get data: %.2f, %.2f\n", data1, data2);
        
        // Sleep for 2 seconds
        sleep(2);
    }
    
    pthread_exit(NULL);
}

// Broadcast data to all connected clients
void broadcast_data_to_clients(double data1, double data2) {
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "%.2f,%.2f", data1, data2);
    
    pthread_mutex_lock(&g_clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] != NULL && g_clients[i]->ssl != NULL) {
            int ret = wolfSSL_write(g_clients[i]->ssl, message, strlen(message));
            if (ret <= 0) {
                printf("[Client %d] Failed to send data\n", g_clients[i]->client_id);
            }
        }
    }
    pthread_mutex_unlock(&g_clients_mutex);
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    g_server_running = 0;
}

// Thread function to handle client connections
void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    WOLFSSL* ssl = NULL;
    char buffer[BUFFER_SIZE];
    int ret;
    int client_slot = -1;
    
    printf("[Client %d] Connected from %s:%d\n", 
           client->client_id,
           inet_ntoa(client->addr.sin_addr), 
           ntohs(client->addr.sin_port));

    // Create SSL object
    ssl = wolfSSL_new(g_ctx);
    if (ssl == NULL) {
        fprintf(stderr, "[Client %d] Error creating SSL object\n", client->client_id);
        goto cleanup;
    }

    // Associate socket with SSL
    wolfSSL_set_fd(ssl, client->sockfd);

    // Perform TLS handshake
    ret = wolfSSL_accept(ssl);
    if (ret != SSL_SUCCESS) {
        int error = wolfSSL_get_error(ssl, ret);
        char error_string[80];
        wolfSSL_ERR_error_string(error, error_string);
        fprintf(stderr, "[Client %d] TLS handshake failed: %s\n", client->client_id, error_string);
        goto cleanup;
    }

    printf("[Client %d] TLS handshake completed successfully!\n", client->client_id);

    // Store SSL object in client structure
    client->ssl = ssl;

    // Add client to global client list
    pthread_mutex_lock(&g_clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] == NULL) {
            g_clients[i] = client;
            client_slot = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_clients_mutex);

    if (client_slot == -1) {
        fprintf(stderr, "[Client %d] Failed to add client to list\n", client->client_id);
        goto cleanup;
    }

    // Get client certificate information
    WOLFSSL_X509* client_cert = wolfSSL_get_peer_certificate(ssl);
    if (client_cert) {
        char* subject = wolfSSL_X509_NAME_oneline(
            wolfSSL_X509_get_subject_name(client_cert), 0, 0);
        printf("[Client %d] Certificate subject: %s\n", client->client_id, subject);
        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        wolfSSL_X509_free(client_cert);
    }

    // Display cipher suite information
    printf("[Client %d] Cipher suite: %s\n", client->client_id, wolfSSL_get_cipher(ssl));
    printf("[Client %d] Protocol version: %s\n", client->client_id, wolfSSL_get_version(ssl));
    printf("[Client %d] Ready to receive data broadcasts\n", client->client_id);

    // Keep connection alive and wait for disconnection
    while (g_server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        ret = wolfSSL_read(ssl, buffer, BUFFER_SIZE - 1);
        
        if (ret > 0) {
            // Client sent some data, just acknowledge
            printf("[Client %d] Received: %s\n", client->client_id, buffer);
        } else if (ret == 0) {
            printf("[Client %d] Disconnected\n", client->client_id);
            break;
        } else {
            int error = wolfSSL_get_error(ssl, ret);
            if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                usleep(100000); // Sleep 100ms to avoid busy waiting
                continue;
            }
            printf("[Client %d] Connection lost\n", client->client_id);
            break;
        }
    }

cleanup:
    // Remove client from global list
    if (client_slot != -1) {
        pthread_mutex_lock(&g_clients_mutex);
        g_clients[client_slot] = NULL;
        pthread_mutex_unlock(&g_clients_mutex);
    }
    
    // Cleanup
    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    close(client->sockfd);
    
    // Update client count
    pthread_mutex_lock(&g_client_count_mutex);
    g_client_count--;
    printf("[Client %d] Connection closed. Active clients: %d\n", client->client_id, g_client_count);
    pthread_mutex_unlock(&g_client_count_mutex);
    
    free(client);
    pthread_exit(NULL);
}

int main() {
    int sockfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pthread_t thread_id;
    int client_id_counter = 0;

    // Initialize random seed
    srand(time(NULL));
    
    // Initialize client list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        g_clients[i] = NULL;
    }

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize wolfSSL
    wolfSSL_Init();

    // Create SSL context
    g_ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (g_ctx == NULL) {
        fprintf(stderr, "Error creating SSL context\n");
        return -1;
    }

    // Load server certificate
    if (wolfSSL_CTX_use_certificate_file(g_ctx, SERVER_CERT, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading server certificate\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Load server private key
    if (wolfSSL_CTX_use_PrivateKey_file(g_ctx, SERVER_KEY, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading server private key\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Load CA certificate for client verification
    if (wolfSSL_CTX_load_verify_locations(g_ctx, CA_CERT, NULL) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Enable mutual authentication (require client certificate)
    wolfSSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Listen for connections
    if (listen(sockfd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    printf("Multi-threaded TLS Server listening on port %d...\n", PORT);
    printf("Maximum concurrent clients: %d\n", MAX_CLIENTS);
    printf("Starting data generation thread...\n");
    
    // Start data generation thread
    if (pthread_create(&g_data_thread, NULL, data_generator, NULL) != 0) {
        fprintf(stderr, "Failed to create data generation thread\n");
        close(sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }
    
    printf("Waiting for client connections... (Press Ctrl+C to stop)\n");

    // Main server loop
    while (g_server_running) {
        client_len = sizeof(client_addr);
        connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        
        if (connfd < 0) {
            if (g_server_running) {
                perror("Accept failed");
            }
            continue;
        }

        // Check if we've reached the maximum number of clients
        pthread_mutex_lock(&g_client_count_mutex);
        if (g_client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&g_client_count_mutex);
            printf("Maximum clients reached (%d), rejecting connection from %s:%d\n",
                   MAX_CLIENTS, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            close(connfd);
            continue;
        }
        g_client_count++;
        client_id_counter++;
        int current_client_id = client_id_counter;
        printf("New connection accepted. Active clients: %d/%d\n", g_client_count, MAX_CLIENTS);
        pthread_mutex_unlock(&g_client_count_mutex);

        // Create client info structure
        client_info_t* client_info = malloc(sizeof(client_info_t));
        if (client_info == NULL) {
            fprintf(stderr, "Memory allocation failed for client info\n");
            close(connfd);
            pthread_mutex_lock(&g_client_count_mutex);
            g_client_count--;
            pthread_mutex_unlock(&g_client_count_mutex);
            continue;
        }
        
        client_info->sockfd = connfd;
        client_info->addr = client_addr;
        client_info->client_id = current_client_id;

        // Create thread to handle client
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_info) != 0) {
            fprintf(stderr, "Thread creation failed\n");
            close(connfd);
            free(client_info);
            pthread_mutex_lock(&g_client_count_mutex);
            g_client_count--;
            pthread_mutex_unlock(&g_client_count_mutex);
            continue;
        }

        // Detach thread so it cleans up automatically
        pthread_detach(thread_id);
    }

    // Cleanup
    printf("\nShutting down server...\n");
    close(sockfd);
    
    // Wait for data generation thread to finish
    printf("Stopping data generation thread...\n");
    pthread_join(g_data_thread, NULL);
    
    // Wait for all client threads to finish
    printf("Waiting for all client connections to close...\n");
    while (g_client_count > 0) {
        usleep(100000); // Sleep for 100ms
    }
    
    wolfSSL_CTX_free(g_ctx);
    wolfSSL_Cleanup();
    pthread_mutex_destroy(&g_client_count_mutex);
    pthread_mutex_destroy(&g_data_mutex);
    pthread_mutex_destroy(&g_clients_mutex);
    
    printf("Server shutdown complete.\n");
    return 0;
}
