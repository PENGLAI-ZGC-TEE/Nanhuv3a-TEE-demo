#include "client.h"

// 全局变量定义
volatile int g_client_running = 1;
WOLFSSL* g_ssl = NULL;
int g_actual_http_port = HTTP_PORT;  // 实际使用的HTTP端口

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down client...\n", sig);
    g_client_running = 0;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [server_ip]\n", program_name);
    printf("  server_ip: IP address of the TLS server (default: %s)\n", DEFAULT_SERVER_IP);
    printf("  Example: %s 192.168.1.100\n", program_name);
    printf("\n");
    printf("The client will:\n");
    printf("  1. Connect to TLS server on port %d to receive sensor data\n", TLS_PORT);
    printf("  2. Start HTTP server on port %d (or next available port)\n", HTTP_PORT);
    printf("  3. Provide API endpoint and web interface on the HTTP server\n");
    printf("  4. Display actual port numbers when server starts\n");
}

int main(int argc, char* argv[]) {
    const char* server_ip = DEFAULT_SERVER_IP;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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

    printf("=== Nuclear Power Plant Monitoring Client ===\n");
    printf("TLS Server: %s:%d\n", server_ip, TLS_PORT);
    printf("===============================================\n\n");

    // Initialize data storage
    init_data_storage();

    // Initialize HTTP server
    if (http_server_init() != 0) {
        fprintf(stderr, "Failed to initialize HTTP server\n");
        cleanup_data_storage();
        return -1;
    }

    // Initialize TLS client
    if (tls_client_init(server_ip) != 0) {
        fprintf(stderr, "Failed to initialize TLS client\n");
        cleanup_data_storage();
        return -1;
    }

    printf("\n=== Client Ready ===\n");
    printf("✓ TLS connection established\n");
    printf("✓ HTTP server running on port %d\n", g_actual_http_port);
    printf("✓ Data storage initialized\n");
    printf("\nPress Ctrl+C to exit\n");
    printf("Open http://localhost:%d/ in your browser to view the monitoring dashboard\n\n", g_actual_http_port);

    // Keep main thread alive
    while (g_client_running) {
        sleep(1);
    }

    printf("\n=== Shutting Down ===\n");

    // Cleanup in reverse order
    tls_client_cleanup();
    cleanup_data_storage();

    printf("Client shutdown completed.\n");
    return 0;
}