#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8900
#define BUF_SIZE 2048

void print_usage(char *cmd) {
    fprintf(stderr, " %s usage:\n", cmd);
    fprintf(stderr, "%s IP_Addr [port]\n", cmd);
}

int main(int argc, char **argv) {
    struct sockaddr_in server;
    int sockfd;
    int len;
    int port = PORT;
    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];

    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        exit(1);
    }

    if (argc == 3) {
        port = atoi(argv[2]);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("can not create socket");
        exit(1);
    }

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);
    printf("What words do you want to tell to server :\n");

    while (1) {
        
        fgets(send_buf, BUF_SIZE, stdin);

        if (strncmp(send_buf, "exit", 4) == 0) {
            break;
        }

        len = sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&server, sizeof(server));
        if (len == -1) {
            perror("send data error");
            close(sockfd);
            exit(1);
        }

        socklen_t server_len = sizeof(server);
        len = recvfrom(sockfd, recv_buf, BUF_SIZE, 0, (struct sockaddr*)&server, &server_len);
        if (len == -1) {
            perror("recv data error");
            close(sockfd);
            exit(1);
        }

        recv_buf[len] = '\0';
        printf("The message from the server is: %s\n", recv_buf);
    }

    close(sockfd);
    return 0;
}
