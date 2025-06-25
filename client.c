#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

#define DEFAULT_SERVER_IP "127.0.0.1"
#define PORT 8443
#define BUFFER_SIZE 1024

// Certificate and key file paths
#define CLIENT_CERT "client-cert.pem"
#define CLIENT_KEY "client-key.pem"
#define CA_CERT "ca-cert.pem"

void print_usage(const char* program_name) {
    printf("Usage: %s [server_ip]\n", program_name);
    printf("  server_ip: IP address of the server (default: %s)\n", DEFAULT_SERVER_IP);
    printf("  Example: %s 192.168.1.100\n", program_name);
}

int main(int argc, char* argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int ret;
    const char* server_ip = DEFAULT_SERVER_IP;

    // Parse command line arguments
    if (argc > 2) {
        printf("Error: Too many arguments\n\n");
        print_usage(argv[0]);
        return -1;
    }
    
    if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        server_ip = argv[1];
    }

    printf("Connecting to server: %s:%d\n", server_ip, PORT);

    // Initialize wolfSSL
    wolfSSL_Init();

    // Create SSL context
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (ctx == NULL) {
        fprintf(stderr, "Error creating SSL context\n");
        return -1;
    }

    // Load client certificate
    if (wolfSSL_CTX_use_certificate_file(ctx, CLIENT_CERT, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading client certificate\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Load client private key
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, CLIENT_KEY, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading client private key\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Load CA certificate for server verification
    if (wolfSSL_CTX_load_verify_locations(ctx, CA_CERT, NULL) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Enable server certificate verification
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        close(sockfd);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sockfd);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    printf("Connected to server %s:%d\n", server_ip, PORT);

    // Create SSL object
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "Error creating SSL object\n");
        close(sockfd);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Associate socket with SSL
    wolfSSL_set_fd(ssl, sockfd);

    // Perform TLS handshake
    ret = wolfSSL_connect(ssl);
    if (ret != SSL_SUCCESS) {
        int error = wolfSSL_get_error(ssl, ret);
        char error_string[80];
        wolfSSL_ERR_error_string(error, error_string);
        fprintf(stderr, "TLS handshake failed: %s\n", error_string);
        wolfSSL_free(ssl);
        close(sockfd);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    printf("TLS handshake completed successfully!\n");

    // Get server certificate information
    WOLFSSL_X509* server_cert = wolfSSL_get_peer_certificate(ssl);
    if (server_cert) {
        char* subject = wolfSSL_X509_NAME_oneline(
            wolfSSL_X509_get_subject_name(server_cert), 0, 0);
        printf("Server certificate subject: %s\n", subject);
        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        wolfSSL_X509_free(server_cert);
    }

    // Display cipher suite information
    printf("Cipher suite: %s\n", wolfSSL_get_cipher(ssl));
    printf("Protocol version: %s\n", wolfSSL_get_version(ssl));
    printf("\n");

    // Communication loop
    printf("Enter messages to send to server (type 'quit' to exit):\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline character
        message[strcspn(message, "\n")] = 0;
        
        // Check for quit command
        if (strcmp(message, "quit") == 0) {
            break;
        }
        
        // Send message to server
        ret = wolfSSL_write(ssl, message, strlen(message));
        if (ret <= 0) {
            int error = wolfSSL_get_error(ssl, ret);
            char error_string[80];
            wolfSSL_ERR_error_string(error, error_string);
            fprintf(stderr, "Error sending message: %s\n", error_string);
            break;
        }
        
        // Receive response from server
        memset(buffer, 0, BUFFER_SIZE);
        ret = wolfSSL_read(ssl, buffer, BUFFER_SIZE - 1);
        
        if (ret > 0) {
            printf("Server response: %s\n\n", buffer);
        } else if (ret == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            int error = wolfSSL_get_error(ssl, ret);
            if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                continue;
            }
            char error_string[80];
            wolfSSL_ERR_error_string(error, error_string);
            fprintf(stderr, "Error receiving response: %s\n", error_string);
            break;
        }
    }

    // Cleanup
    printf("Closing connection...\n");
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
