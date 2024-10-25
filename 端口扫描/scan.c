#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>  // 用于 getservbyport

#define TIMEOUT 1 // 设置超时时间为 1 秒

// 扫描端口函数
int scan_port(const char *ip, int port) {
    int sock;
    struct sockaddr_in server;
    socklen_t optlen = sizeof(int);
    int result;

    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    // 设置超时时间
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    // 配置服务器地址
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 尝试连接到端口
    result = connect(sock, (struct sockaddr*)&server, sizeof(server));
    close(sock);

    return (result == 0) ? 1 : 0; // 返回1表示端口开放，0表示端口关闭
}

// 获取服务名
const char *get_service_name(int port) {
    struct servent *service = getservbyport(htons(port), NULL);
    if (service) {
        return service->s_name; // 返回服务名
    } else {
        return "unknown"; // 如果找不到服务，返回 "unknown"
    }
}

int main() {
    char target_ip[16];
    int start_port, end_port;

    // 获取用户输入
    printf("target ip: ");
    scanf("%s", target_ip);
    printf("start port: ");
    scanf("%d", &start_port);
    printf("end port: ");
    scanf("%d", &end_port);

    
    int port;  //循环扫描端口
    for (port = start_port; port <= end_port; port++) {
        if (scan_port(target_ip, port)) {
            printf("port: %d open, service: %s\n", port, get_service_name(port));
        }
    }

    return 0;
}
