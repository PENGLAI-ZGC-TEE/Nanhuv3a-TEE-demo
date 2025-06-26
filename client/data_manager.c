#include "client.h"

// 全局数据存储
sensor_data_t* g_sensor_data = NULL;
int g_data_count = 0;
int g_data_capacity = 0;
pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_data_storage(void) {
    pthread_mutex_lock(&g_data_mutex);
    
    g_data_capacity = MAX_DATA_POINTS;
    g_sensor_data = malloc(g_data_capacity * sizeof(sensor_data_t));
    g_data_count = 0;
    
    if (!g_sensor_data) {
        fprintf(stderr, "Failed to allocate memory for sensor data\n");
        g_data_capacity = 0;
    } else {
        printf("Data storage initialized with capacity for %d data points\n", g_data_capacity);
    }
    
    pthread_mutex_unlock(&g_data_mutex);
}

void add_sensor_data(double centrifuge_speed, double power_output) {
    pthread_mutex_lock(&g_data_mutex);
    
    if (!g_sensor_data || g_data_capacity == 0) {
        pthread_mutex_unlock(&g_data_mutex);
        return;
    }
    
    // If we've reached capacity, remove the oldest data point
    if (g_data_count >= g_data_capacity) {
        // Shift all data points left by one position
        memmove(&g_sensor_data[0], &g_sensor_data[1], 
                (g_data_capacity - 1) * sizeof(sensor_data_t));
        g_data_count = g_data_capacity - 1;
    }
    
    // Add new data point
    sensor_data_t* new_data = &g_sensor_data[g_data_count];
    new_data->centrifuge_speed = centrifuge_speed;
    new_data->power_output = power_output;
    
    // Generate timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    // strftime(new_data->timestamp, sizeof(new_data->timestamp), 
    //          "%Y-%m-%d %H:%M:%S", tm_info);
    strftime(new_data->timestamp, sizeof(new_data->timestamp), 
             "%H:%M:%S", tm_info);
    
    g_data_count++;
    
    printf("Added sensor data: Centrifuge=%.1f RPM, Power=%.1f MW, Time=%s\n",
           centrifuge_speed, power_output, new_data->timestamp);
    
    pthread_mutex_unlock(&g_data_mutex);
}

char* get_sensor_data_json(void) {
    pthread_mutex_lock(&g_data_mutex);
    
    if (!g_sensor_data || g_data_count == 0) {
        pthread_mutex_unlock(&g_data_mutex);
        char* empty_json = malloc(64);
        if (empty_json) {
            strcpy(empty_json, "{\"data\":[],\"count\":0,\"message\":\"No data available\"}");
        }
        return empty_json;
    }
    
    // Calculate required buffer size
    // Each data point needs approximately 120 characters in JSON format
    size_t buffer_size = 1024 + (g_data_count * 150);
    char* json_buffer = malloc(buffer_size);
    
    if (!json_buffer) {
        pthread_mutex_unlock(&g_data_mutex);
        return NULL;
    }
    
    // Start building JSON
    strcpy(json_buffer, "{\"data\":[");
    
    for (int i = 0; i < g_data_count; i++) {
        char data_item[200];
        snprintf(data_item, sizeof(data_item),
            "%s{\"centrifugeSpeed\":%.1f,\"powerOutput\":%.1f,\"timestamp\":\"%s\"}",
            (i > 0) ? "," : "",
            g_sensor_data[i].centrifuge_speed,
            g_sensor_data[i].power_output,
            g_sensor_data[i].timestamp);
        
        strcat(json_buffer, data_item);
    }
    
    // Add metadata
    char metadata[200];
    snprintf(metadata, sizeof(metadata),
        "],\"count\":%d,\"capacity\":%d,\"message\":\"Data retrieved successfully\"}",
        g_data_count, g_data_capacity);
    
    strcat(json_buffer, metadata);
    
    pthread_mutex_unlock(&g_data_mutex);
    return json_buffer;
}

void cleanup_data_storage(void) {
    pthread_mutex_lock(&g_data_mutex);
    
    if (g_sensor_data) {
        free(g_sensor_data);
        g_sensor_data = NULL;
    }
    
    g_data_count = 0;
    g_data_capacity = 0;
    
    printf("Data storage cleaned up\n");
    
    pthread_mutex_unlock(&g_data_mutex);
    pthread_mutex_destroy(&g_data_mutex);
}