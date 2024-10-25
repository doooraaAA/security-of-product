#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define TIMEOUT 1 // 超时时间
#define MAX_PORT 65535 // 最大端口号
#define NUM_PORTS 100 // 每个IP扫描的线程数
#define MAX_IPS 5 // 最大IP数量
#define START_PORT 1 // 开始扫描的端口
#define END_PORT 1024 // 默认扫描端口范围

typedef struct {
    char ip[16];
    int port;
    int thread_num; // 当前线程的编号
} scan_params;

int scan_port(const char *ip, int port) {
    int sock;
    struct sockaddr_in server;
    int result;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    result = connect(sock, (struct sockaddr*)&server, sizeof(server));
    close(sock);

    return (result == 0) ? 1 : 0; // 返回1表示端口开放
}

const char *get_service_name(int port) {
    struct servent *service = getservbyport(htons(port), NULL);
    return service ? service->s_name : "unknown"; // 如果找不到服务，返回 "unknown"
}

void* scan_thread(void *args) {
    scan_params *params = (scan_params *)args;
    const char *ip = params->ip;
    int port = params->port;
    int thread_num = params->thread_num; // 获取线程编号

    if (scan_port(ip, port)) {
        printf("Thread %d: IP: %s, Port: %d, Service: %s\n", thread_num, ip, port, get_service_name(port));
    }

    free(params); // 释放参数内存
    return NULL;
}

int main() {
    char ips[MAX_IPS][16];
    int i;

    for (i = 0; i < MAX_IPS; i++) {
        printf("please input the %dth IP address (enter 'end' to finish): ", i + 1);
        scanf("%s", ips[i]);
        if (strcmp(ips[i], "end") == 0) {
            break;
        }
    }

    for (i = 0; i < MAX_IPS && strlen(ips[i]) > 0; i++) {
        pthread_t threads[NUM_PORTS];
        int thread_count = 0;
        int j;
        for (j = START_PORT; j <= END_PORT; j++) {
            scan_params *params = malloc(sizeof(scan_params));
            if (params == NULL) {
                perror("Failed to allocate memory");
                exit(EXIT_FAILURE);
            }
            strcpy(params->ip, ips[i]);
            params->port = j;
            params->thread_num = thread_count + 1; // 设置当前线程编号

            pthread_create(&threads[thread_count++], NULL, scan_thread, params);

            // 每 NUM_PORTS 个线程后加入等候
            if (thread_count >= NUM_PORTS) {
                int k;
                for (k = 0; k < thread_count; k++) {
                    pthread_join(threads[k], NULL);
                }
                thread_count = 0; // 重置线程计数
            }
        }
        
        // 等待剩余的线程
        int k;
        for (k = 0; k < thread_count; k++) {
            pthread_join(threads[k], NULL);
        }
    }

    return 0;
}
