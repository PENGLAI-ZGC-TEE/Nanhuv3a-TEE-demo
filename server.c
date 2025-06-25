#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

#define PORT 8443
#define BUFFER_SIZE 1024

// Certificate and key file paths
#define SERVER_CERT "certs/server-cert.pem"
#define SERVER_KEY "certs/server-key.pem"
#define CA_CERT "certs/ca-cert.pem"

int main() {
    int sockfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    char buffer[BUFFER_SIZE];
    int ret;

    // Initialize wolfSSL
    wolfSSL_Init();

    // Create SSL context
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "Error creating SSL context\n");
        return -1;
    }

    // Load server certificate
    if (wolfSSL_CTX_use_certificate_file(ctx, SERVER_CERT, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading server certificate\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Load server private key
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, SERVER_KEY, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading server private key\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Load CA certificate for client verification
    if (wolfSSL_CTX_load_verify_locations(ctx, CA_CERT, NULL) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Enable mutual authentication (require client certificate)
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        wolfSSL_CTX_free(ctx);
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
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        close(sockfd);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    printf("TLS Server listening on port %d...\n", PORT);
    printf("Waiting for client connections...\n");

    while (1) {
        client_len = sizeof(client_addr);
        connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create SSL object
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            fprintf(stderr, "Error creating SSL object\n");
            close(connfd);
            continue;
        }

        // Associate socket with SSL
        wolfSSL_set_fd(ssl, connfd);

        // Perform TLS handshake
        ret = wolfSSL_accept(ssl);
        if (ret != SSL_SUCCESS) {
            int error = wolfSSL_get_error(ssl, ret);
            char error_string[80];
            wolfSSL_ERR_error_string(error, error_string);
            fprintf(stderr, "TLS handshake failed: %s\n", error_string);
            wolfSSL_free(ssl);
            close(connfd);
            continue;
        }

        printf("TLS handshake completed successfully!\n");

        // Get client certificate information
        WOLFSSL_X509* client_cert = wolfSSL_get_peer_certificate(ssl);
        if (client_cert) {
            char* subject = wolfSSL_X509_NAME_oneline(
                wolfSSL_X509_get_subject_name(client_cert), 0, 0);
            printf("Client certificate subject: %s\n", subject);
            XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
            wolfSSL_X509_free(client_cert);
        }

        // Communication loop
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            ret = wolfSSL_read(ssl, buffer, BUFFER_SIZE - 1);
            
            if (ret > 0) {
                printf("Received: %s\n", buffer);
                
                // Echo back the message
                char response[BUFFER_SIZE];
                snprintf(response, BUFFER_SIZE, "Server echo: %s", buffer);
                
                ret = wolfSSL_write(ssl, response, strlen(response));
                if (ret <= 0) {
                    fprintf(stderr, "Error writing to SSL connection\n");
                    break;
                }
            } else if (ret == 0) {
                printf("Client disconnected\n");
                break;
            } else {
                int error = wolfSSL_get_error(ssl, ret);
                if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                    continue;
                }
                fprintf(stderr, "Error reading from SSL connection\n");
                break;
            }
        }

        // Cleanup
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        close(connfd);
        printf("Connection closed\n\n");
    }

    // Cleanup
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}