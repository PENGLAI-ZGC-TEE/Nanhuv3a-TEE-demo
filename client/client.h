#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

// 配置常量
#define DEFAULT_SERVER_IP "127.0.0.1"
#define TLS_PORT 8443
#define HTTP_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_DATA_POINTS 50
#define API_RESPONSE_SIZE 8192

// 证书路径
#define CLIENT_CERT "certs/client-cert.pem"
#define CLIENT_KEY "certs/client-key.pem"
#define CA_CERT "certs/ca-cert.pem"

// 传感器数据结构
typedef struct {
    double centrifuge_speed;  // 离心机转速
    double power_output;      // 发电量
    char timestamp[32];       // 时间戳
} sensor_data_t;

// 全局变量声明
extern volatile int g_client_running;
extern WOLFSSL* g_ssl;
extern sensor_data_t* g_sensor_data;
extern int g_data_count;
extern int g_data_capacity;
extern pthread_mutex_t g_data_mutex;
extern int g_actual_http_port;  // 实际使用的HTTP端口

// TLS客户端函数
int tls_client_init(const char* server_ip);
void* tls_data_receiver(void* arg);
void tls_client_cleanup(void);

// HTTP服务器函数
int http_server_init(void);
void* http_server_thread(void* arg);
void handle_http_request(int client_socket);
void send_http_response(int client_socket, const char* status, const char* content_type, const char* body);
void send_api_data(int client_socket);
void send_static_file(int client_socket, const char* path);

// 数据管理函数
void add_sensor_data(double centrifuge_speed, double power_output);
char* get_sensor_data_json(void);
void init_data_storage(void);
void cleanup_data_storage(void);

// 工具函数
void signal_handler(int sig);
void print_usage(const char* program_name);
const char* get_mime_type(const char* path);

#endif // CLIENT_H