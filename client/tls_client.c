#include "client.h"

static WOLFSSL_CTX* g_ctx = NULL;
static int g_sockfd = -1;
static pthread_t g_receiver_thread;

int tls_client_init(const char* server_ip) {
    struct sockaddr_in server_addr;
    int ret;

    printf("Connecting to TLS server: %s:%d\n", server_ip, TLS_PORT);

    // Initialize wolfSSL
    wolfSSL_Init();

    // Create SSL context
    g_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (g_ctx == NULL) {
        fprintf(stderr, "Error creating SSL context\n");
        return -1;
    }

    // Load client certificate
    if (wolfSSL_CTX_use_certificate_file(g_ctx, CLIENT_CERT, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading client certificate\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Load client private key
    if (wolfSSL_CTX_use_PrivateKey_file(g_ctx, CLIENT_KEY, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading client private key\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Load CA certificate for server verification
    if (wolfSSL_CTX_load_verify_locations(g_ctx, CA_CERT, NULL) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate\n");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Enable server certificate verification
    wolfSSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER, NULL);

    // Create socket
    g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sockfd < 0) {
        perror("Socket creation failed");
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TLS_PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        close(g_sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Connect to server
    if (connect(g_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(g_sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    printf("Connected to TLS server %s:%d\n", server_ip, TLS_PORT);

    // Create SSL object
    g_ssl = wolfSSL_new(g_ctx);
    if (g_ssl == NULL) {
        fprintf(stderr, "Error creating SSL object\n");
        close(g_sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    // Associate socket with SSL
    wolfSSL_set_fd(g_ssl, g_sockfd);

    // Perform TLS handshake
    ret = wolfSSL_connect(g_ssl);
    if (ret != SSL_SUCCESS) {
        int error = wolfSSL_get_error(g_ssl, ret);
        char error_string[80];
        wolfSSL_ERR_error_string(error, error_string);
        fprintf(stderr, "TLS handshake failed: %s\n", error_string);
        wolfSSL_free(g_ssl);
        close(g_sockfd);
        wolfSSL_CTX_free(g_ctx);
        return -1;
    }

    printf("TLS handshake completed successfully!\n");

    // Get server certificate information
    WOLFSSL_X509* server_cert = wolfSSL_get_peer_certificate(g_ssl);
    if (server_cert) {
        char* subject = wolfSSL_X509_NAME_oneline(
            wolfSSL_X509_get_subject_name(server_cert), 0, 0);
        printf("Server certificate subject: %s\n", subject);
        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        wolfSSL_X509_free(server_cert);
    }

    // Display cipher suite information
    printf("Cipher suite: %s\n", wolfSSL_get_cipher(g_ssl));
    printf("Protocol version: %s\n", wolfSSL_get_version(g_ssl));
    printf("\n");

    // Start data receiver thread
    if (pthread_create(&g_receiver_thread, NULL, tls_data_receiver, NULL) != 0) {
        fprintf(stderr, "Failed to create TLS receiver thread\n");
        return -1;
    }

    printf("TLS client initialized and ready to receive data.\n");
    return 0;
}

void* tls_data_receiver(void* arg) {
    (void)arg; // Suppress unused parameter warning
    char buffer[BUFFER_SIZE];
    int ret;
    
    while (g_client_running && g_ssl != NULL) {
        memset(buffer, 0, BUFFER_SIZE);
        ret = wolfSSL_read(g_ssl, buffer, BUFFER_SIZE - 1);
        
        if (ret > 0) {
            printf("Received TLS data: %s\n", buffer);
            
            // Parse the received data (format: "centrifuge_speed,power_output")
            double centrifuge_speed, power_output;
            if (sscanf(buffer, "%lf,%lf", &centrifuge_speed, &power_output) == 2) {
                add_sensor_data(centrifuge_speed, power_output);
            } else {
                printf("Warning: Invalid data format received: %s\n", buffer);
            }
        } else if (ret == 0) {
            printf("TLS server disconnected\n");
            g_client_running = 0;
            break;
        } else {
            int error = wolfSSL_get_error(g_ssl, ret);
            if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                usleep(100000); // Sleep 100ms to avoid busy waiting
                continue;
            }
            printf("TLS connection lost\n");
            g_client_running = 0;
            break;
        }
    }
    
    pthread_exit(NULL);
}

void tls_client_cleanup(void) {
    printf("Cleaning up TLS client...\n");
    
    // Wait for receiver thread to finish
    if (g_receiver_thread != 0) {
        printf("Waiting for TLS receiver thread to finish...\n");
        pthread_join(g_receiver_thread, NULL);
    }
    
    // Cleanup SSL
    if (g_ssl) {
        wolfSSL_shutdown(g_ssl);
        wolfSSL_free(g_ssl);
        g_ssl = NULL;
    }
    
    // Close socket
    if (g_sockfd >= 0) {
        close(g_sockfd);
        g_sockfd = -1;
    }
    
    // Cleanup SSL context
    if (g_ctx) {
        wolfSSL_CTX_free(g_ctx);
        g_ctx = NULL;
    }
    
    wolfSSL_Cleanup();
    printf("TLS client cleanup completed.\n");
}