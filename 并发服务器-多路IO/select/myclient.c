#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>

#define CLIENTPORT 1573
#define BUFSIZE 2048

int main(int argc, char *argv[]) {
    int sockfd;
    fd_set sockset;
    struct sockaddr_in serveraddr;

    char recv_buf[BUFSIZE];
    char send_buf[BUFSIZE];
    char username[50]; // 用于存储用户名

    if (2 > argc) {
        printf("Please input the server IP!\n");
        exit(1);
    }

    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("create socket error");
        exit(1);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(CLIENTPORT);

    if (-1 == connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) {
        perror("connect error");
        exit(1);
    }

    fprintf(stderr, "Welcome to the chat server.\n");
    fprintf(stderr, "Please input your name: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0'; // 去掉换行符

    if (0 >= send(sockfd, username, strlen(username), 0)) {
        perror("sending data error");
        close(sockfd);
        exit(1);
    }

    FD_ZERO(&sockset);
    FD_SET(sockfd, &sockset);
    FD_SET(STDIN_FILENO, &sockset);

    printf("Client is connected!\n");

    while (1) {
        fd_set readset = sockset; // 新的读取集合
        if (select(sockfd + 1, &readset, NULL, NULL, NULL) < 0) {
            perror("select error");
            close(sockfd);
            exit(1);
        }

        // 处理服务器消息
        if (FD_ISSET(sockfd, &readset)) {
            int recvbytes = read(sockfd, recv_buf, sizeof(recv_buf) - 1);
            if (recvbytes <= 0) { // 服务器断开连接
                perror("read data error");
                close(sockfd);
                exit(1);
            }
            recv_buf[recvbytes] = '\0';
            printf("%s\n", recv_buf);
            fflush(stdout);
        }

        // 处理标准输入
        if (FD_ISSET(STDIN_FILENO, &readset)) {
            fgets(send_buf, sizeof(send_buf), stdin);
            send_buf[strcspn(send_buf, "\n")] = '\0'; // 去掉换行符

            // 处理命令
            if (strncmp(send_buf, "/quit", 5) == 0) {
                send(sockfd, send_buf, strlen(send_buf), 0);
                close(sockfd);
                exit(0);
            }

            // 处理/send命令
            if (strncmp(send_buf, "/send", 5) == 0) {
                char message[BUFSIZE];
                if (strlen(send_buf) > 5) {
                    sprintf(message, "%s", send_buf); // 直接发送完整的/send命令
                } else {
                    printf("Usage: /send <username> <message>\n");
                    continue; // 提示用户输入格式错误并继续询问
                }
                
                if (0 >= send(sockfd, message, strlen(message), 0)) {
                    perror("send data error");
                    close(sockfd);
                    exit(1);
                }
            } else if (strncmp(send_buf, "/help", 5) == 0 || strncmp(send_buf, "/who", 4) == 0) {
                if (0 >= send(sockfd, send_buf, strlen(send_buf), 0)) {
                    perror("send data error");
                    close(sockfd);
                    exit(1);
                }
            } else {
                // 默认广播消息
                char message[BUFSIZE];
                sprintf(message, "%s: %s", username, send_buf);
                if (0 >= send(sockfd, message, strlen(message), 0)) {
                    perror("send data error");
                    close(sockfd);
                    exit(1);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
