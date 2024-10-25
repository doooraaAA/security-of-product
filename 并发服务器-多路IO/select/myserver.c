#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVERPORT 1573
#define BACKLOG 10
#define BUFSIZE 2048

struct client_info {
    int client_id;
    struct sockaddr_in client_address;
    char name[50]; // 用户名
};

struct client_info clients[BACKLOG];
int client_count = 0;

void broadcast(char *message, int sender_fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].client_id != sender_fd) {
            send(clients[i].client_id, message, strlen(message), 0);
        }
    }
}

void handle_command(char *command, int client_fd) {
    char message[BUFSIZE];

    if (strncmp(command, "/quit", 5) == 0) {
        close(client_fd);
        for (int j = 0; j < client_count; j++) {
            if (clients[j].client_id == client_fd) {
                sprintf(message, "%s has left the chat.\n", clients[j].name);
                broadcast(message, client_fd);
                clients[j] = clients[--client_count]; // 移除客户端
            }
        }
    } else if (strncmp(command, "/who", 4) == 0) {
        // 列出在线用户
        strcpy(message, "Online users:\n");
        for (int i = 0; i < client_count; i++) {
            strcat(message, clients[i].name);
            strcat(message, "\n");
        }
        send(client_fd, message, strlen(message), 0);
    } else if (strncmp(command, "/send", 5) == 0) {
        // 点对点消息
        char target[50], msg[BUFSIZE];
        sscanf(command + 6, "%s %[^\n]", target, msg);
        
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].name, target) == 0) {
                sprintf(message, "[Private from %s]: %s\n", clients[client_fd].name, msg);
                send(clients[i].client_id, message, strlen(message), 0); // 仅发送给目标用户
                return;
            }
        }
        strcpy(message, "User not found.\n");
        send(client_fd, message, strlen(message), 0);
    } else if (strncmp(command, "/help", 5) == 0) {
        // 显示帮助信息
        char *help_msg = "/help: Show this help message\n/quit: Exit the chat\n/who: List online users\n/send username message: Send a private message";
        send(client_fd, help_msg, strlen(help_msg), 0);
    } else {
        // 广播消息
        sprintf(message, "%s: %s\n", clients[client_fd].name, command);
        broadcast(message, client_fd);
    }
}

int main() {
    fd_set master_fds, read_fds;
    int max_fd, sockfd, newfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buf[BUFSIZE];

    // 创建监听套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(sockfd, BACKLOG);

    FD_ZERO(&master_fds);
    FD_SET(sockfd, &master_fds);
    max_fd = sockfd;
    printf("Chat server started on port %d\n", SERVERPORT);

    while (1) {
        read_fds = master_fds;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) { // 新连接
                    newfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
                    if (newfd < 0) {
                        perror("accept");
                    } else {
                        // 获取用户名
                        recv(newfd, buf, sizeof(buf), 0);
                        strcpy(clients[client_count].name, buf);
                        clients[client_count].client_id = newfd;
                        client_count++;
                        FD_SET(newfd, &master_fds);
                        if (newfd > max_fd) max_fd = newfd;

                        printf("%s has joined the chat.\n", buf);
                        char message[BUFSIZE];
                        sprintf(message, "%s has joined the chat.\n", buf);
                        broadcast(message, newfd);
                    }
                } else { // 处理客户端消息
                    int recvbytes = recv(i, buf, sizeof(buf) - 1, 0);
                    if (recvbytes <= 0) { // 客户端断开连接
                        close(i);
                        FD_CLR(i, &master_fds);
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j].client_id == i) {
                                printf("%s has left the chat.\n", clients[j].name);
                                char message[BUFSIZE];
                                sprintf(message, "%s has left the chat.\n", clients[j].name);
                                broadcast(message, i);
                                clients[j] = clients[--client_count]; // 移除客户端信息
                                break;
                            }
                        }
                    } else {
                        buf[recvbytes] = '\0'; // 确保字符串结束
                        printf("Received message from %s: %s\n", clients[i].name, buf);
                        handle_command(buf, i);
                    }
                }
            }
        }
    }
    return 0;
}
