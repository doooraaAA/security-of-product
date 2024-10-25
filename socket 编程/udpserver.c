#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8900

int main(int argc, char** argv)
{
    struct sockaddr_in server;
    struct sockaddr_in client;
    int client_len = sizeof(struct sockaddr_in);
    int listend;
    int sendnum;
    int recvnum;
    char send_buf[2048];
    char recv_buf[2048];

    memset(send_buf, 0, sizeof(send_buf));
    memset(recv_buf, 0, sizeof(recv_buf));
    
    if ((listend = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("create socket error");
        exit(1);
    }

    int opt = 1;
    setsockopt(listend, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(listend, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind error");
        exit(1);
    }

    while (1) {
        recvnum = recvfrom(listend, recv_buf, sizeof(recv_buf) - 1, 0, (struct sockaddr *)&client, &client_len);
        if (recvnum < 0) {
            perror("recv error");
            continue;
        }
        recv_buf[recvnum] = '\0';

        printf("the message from the client is: %s", recv_buf);
        printf("from client %s\n",inet_ntoa(client.sin_addr));
        printf("\n");

        if (strcmp(recv_buf, "quit") == 0) {
            printf("the client broke the server process\n");
            break;
        }

        sendnum = sprintf(send_buf, "byebye, the guest from %s\n", inet_ntoa(client.sin_addr));
        if (sendto(listend, send_buf, sendnum, 0, (struct sockaddr *)&client, client_len) < 0) {
            perror("send error");
            continue;
        }
    }

    close(listend);
    return 0;
}
