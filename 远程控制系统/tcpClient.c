#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8900

void print_usage(char *cmd)
{
    fprintf(stderr, " %s usage:\n", cmd);
    fprintf(stderr, "%s IP_Addr [port]\n", cmd);
}

int main(int argc, char **argv)
{
    struct sockaddr_in server;
    int ret;
    int len;
    int port;
    int sockfd;
    char send_buf[2048];
    char recv_buf[2048];

    if ((2 > argc) || (argc > 3))
    {
        print_usage(argv[0]);
        exit(1);
    }

    if (3 == argc)
    {
        port = atoi(argv[2]);
    }
    else
    {
        port = PORT;
    }

    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("can not create socket\n");
        exit(1);
    }

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (0 > (ret = connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr))))
    {
        perror("connect error");
        close(sockfd);
        exit(1);
    }

    while (1)
    {
        printf("what words do you want to tell to server:\n");
        fgets(send_buf, sizeof(send_buf), stdin);

        // Remove newline character from fgets
        send_buf[strcspn(send_buf, "\n")] = 0;
        send(sockfd, send_buf, strlen(send_buf), 0);
        if (strcmp(send_buf, "quit") == 0)
        {
            break;
        }

        if (0 > (len = send(sockfd, send_buf, strlen(send_buf), 0)))
        {
            perror("send data error\n");
            close(sockfd);
            exit(1);
        }

        if (0 > (len = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0)))
        {
            perror("recv data error\n");
            close(sockfd);
            exit(1);
        }

        recv_buf[len] = '\0';

        printf("the message from the server is: %s\n", recv_buf);
    }

    close(sockfd);
    return 0;
}
