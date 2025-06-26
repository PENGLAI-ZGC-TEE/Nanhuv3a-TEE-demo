#include "client.h"

static int g_http_sockfd = -1;
static pthread_t g_http_thread;

int http_server_init(void) {
    struct sockaddr_in server_addr;
    int opt = 1;
    int port = HTTP_PORT;
    int max_attempts = 100; // 最多尝试100个端口

    // Create socket
    g_http_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_http_sockfd < 0) {
        perror("HTTP socket creation failed");
        return -1;
    }

    // Set socket options
    if (setsockopt(g_http_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("HTTP setsockopt failed");
        close(g_http_sockfd);
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Try to bind to available port, starting from HTTP_PORT
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        server_addr.sin_port = htons(port);
        
        if (bind(g_http_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            // Bind successful, update global port variable
            g_actual_http_port = port;
            break;
        }
        
        if (attempt == max_attempts - 1) {
            fprintf(stderr, "HTTP bind failed: Unable to find available port after %d attempts\n", max_attempts);
            close(g_http_sockfd);
            return -1;
        }
        
        printf("Port %d is in use, trying port %d...\n", port, port + 1);
        port++;
    }

    // Listen for connections
    if (listen(g_http_sockfd, 10) < 0) {
        perror("HTTP listen failed");
        close(g_http_sockfd);
        return -1;
    }

    printf("HTTP server listening on port %d\n", port);

    // Start HTTP server thread
    if (pthread_create(&g_http_thread, NULL, http_server_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create HTTP server thread\n");
        close(g_http_sockfd);
        return -1;
    }

    printf("HTTP server initialized successfully.\n");
    return 0;
}

void* http_server_thread(void* arg) {
    (void)arg;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket;

    while (g_client_running) {
        client_socket = accept(g_http_sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (g_client_running) {
                perror("HTTP accept failed");
            }
            continue;
        }

        printf("HTTP client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_http_request(client_socket);
        close(client_socket);
    }

    pthread_exit(NULL);
}

void handle_http_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    char method[16], path[256], version[16];
    int bytes_read;

    // Read HTTP request
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        return;
    }
    buffer[bytes_read] = '\0';

    // Parse request line
    if (sscanf(buffer, "%15s %255s %15s", method, path, version) != 3) {
        send_http_response(client_socket, "400 Bad Request", "text/plain", "Bad Request");
        return;
    }

    printf("HTTP Request: %s %s %s\n", method, path, version);

    // Handle different routes
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/api/data") == 0) {
            send_api_data(client_socket);
        } else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            send_static_file(client_socket, "public/index.html");
        } else {
            // Try to serve static file from public directory
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "public%s", path);
            send_static_file(client_socket, file_path);
        }
    } else {
        send_http_response(client_socket, "405 Method Not Allowed", "text/plain", "Method Not Allowed");
    }
}

void send_http_response(int client_socket, const char* status, const char* content_type, const char* body) {
    char response[BUFFER_SIZE * 4];
    int content_length = strlen(body);

    snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        status, content_type, content_length, body);

    send(client_socket, response, strlen(response), 0);
}

void send_api_data(int client_socket) {
    char* json_data = get_sensor_data_json();
    if (json_data) {
        send_http_response(client_socket, "200 OK", "application/json", json_data);
        free(json_data);
    } else {
        send_http_response(client_socket, "500 Internal Server Error", "text/plain", "Failed to generate JSON data");
    }
}

void send_static_file(int client_socket, const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        send_http_response(client_socket, "404 Not Found", "text/plain", "File Not Found");
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        send_http_response(client_socket, "500 Internal Server Error", "text/plain", "Empty file");
        return;
    }

    // Read file content
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        send_http_response(client_socket, "500 Internal Server Error", "text/plain", "Memory allocation failed");
        return;
    }

    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    fclose(file);

    // Determine content type
    const char* content_type = get_mime_type(path);

    // Send response
    char response_header[BUFFER_SIZE];
    snprintf(response_header, sizeof(response_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        content_type, bytes_read);

    send(client_socket, response_header, strlen(response_header), 0);
    send(client_socket, content, bytes_read, 0);

    free(content);
}

const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "text/plain";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";

    return "text/plain";
}