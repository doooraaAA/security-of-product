#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define CLIENTPORT 1573
#define BUFSIZE 2048

int main(int argc, char* argv[]) {
    int sockfd;
    fd_set sockset;
    struct sockaddr_in serveraddr;
    int recvbytes;
    char recv_buf[BUFSIZE];
    char send_buf[BUFSIZE];
    int data_len;

    if (argc < 2) {
        printf("Please input the server IP!\n");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error");
        exit(1);
    }

    memset(&serveraddr, 0, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(CLIENTPORT);

    if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr)) == -1) {
        perror("connect() error");
        exit(1);
    }

    printf("Welcome to the chat server\n");
    printf("Please input your name: ");
    fgets(send_buf, sizeof(send_buf), stdin);

    if (send(sockfd, send_buf, strlen(send_buf), 0) <= 0) {
        perror("send() error");
        close(sockfd);
        exit(1);
    }

    FD_ZERO(&sockset);
    FD_SET(sockfd, &sockset);
    FD_SET(0, &sockset);

    //printf("client ok!\n");

    while (1) {
        fd_set read_fds = sockset;
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select() error");
            exit(1);
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            if ((recvbytes = recv(sockfd, recv_buf, sizeof(recv_buf), 0)) <= 0) {
                if (recvbytes == 0) {
                    printf("Server closed connection\n");
                }
                else {
                    perror("recv() error");
                }
                close(sockfd);
                exit(1);
            }
            else {
                recv_buf[recvbytes] = '\0';
                printf("%s\n", recv_buf);
            }
        }

        if (FD_ISSET(0, &read_fds)) {
            if (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
                data_len = strlen(send_buf);
                if (send_buf[data_len - 1] == '\n') {
                    send_buf[data_len - 1] = '\0';
                }

                if (send(sockfd, send_buf, strlen(send_buf), 0) == -1) {
                    perror("send() error");
                    close(sockfd);
                    exit(1);
                }

                if (strcmp(send_buf, "/quit") == 0) {
                    printf("Quitting the chat room\n");
                    close(sockfd);
                    exit(0);
                }
            }
        }

        FD_ZERO(&sockset);
        FD_SET(sockfd, &sockset);
        FD_SET(0, &sockset);
    }

    close(sockfd);
    return 0;
}