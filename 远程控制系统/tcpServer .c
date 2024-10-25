#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8900
#define BUFSIZE 2048

/**
 * @description: 将客户端内容作为命令执行，并发送给客户端
 * @param:cmd:待处理的命令
 *       :sockfd: 通讯的套接字
 *       :client:对应客户端地址
 * @return {*}：0 正确，-1 失败
 */

int execute(char* cmd, int sockfd)
{
    char buf[BUFSIZE];
    FILE* fp;
    char* errbuf = "command cannot execute\n";
    int ret;
    int counter;

    fp = NULL;
    counter = 0;
    memset(buf, 0, BUFSIZE);

    if ((fp = popen(cmd, "r")) == NULL||counter==0)
    {
        perror("open failed!");
        ret = send(sockfd, errbuf, strlen(errbuf), 0);
        return -1;
    }

    while ((counter < BUFSIZE - 1) && (fgets(buf + counter, BUFSIZE - counter, fp) != NULL))
    {
        counter = strlen(buf);
    }
     // 如果没有输出，则打印指令信息
    if (counter == 0) {
        snprintf(errbuf, sizeof(errbuf), "指令无输出: %s\n", cmd);
        send(sockfd, errbuf, strlen(errbuf), 0);
        pclose(fp);  // 关闭管道
        return -1;
    }
    ret = send(sockfd, buf, counter, 0);
    if (0 > ret)
        return -1;

    return 0;
}

int main(int argc, char** argv)
{
    // declare variable
    int listenfd, connfd;
    struct sockaddr_in serv, client;
    int opt;
    int ret;
    socklen_t len;
    char send_buf[BUFSIZE], recv_buf[BUFSIZE];

    // variable initialization
    listenfd = -1;
    connfd = -1;
    opt = 1;
    ret = -1;
    len = sizeof(struct sockaddr);
    bzero(&serv, sizeof(struct sockaddr));
    bzero(&client, sizeof(struct sockaddr));
    bzero(send_buf, BUFSIZE);
    bzero(recv_buf, BUFSIZE);

    // create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > listenfd)
    {
        perror("error in creating socket");
        exit(-1);
    }

    // set the socket
    // allow re-bind
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    // set the value for serv
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT);

    // bind the socket
    ret = bind(listenfd, (struct sockaddr*)&serv, sizeof(struct sockaddr));
    if (0 > ret)
    {
        perror("error in binding");
        exit(-1);
    }

    // listen for incoming connections
    ret = listen(listenfd, 5);
    if (0 > ret)
    {
        perror("error in listening");
        exit(-1);
    }

    // accept incoming connection and handle communication
    while (1)
    {
        connfd = accept(listenfd, (struct sockaddr*)&client, &len);
        if (0 > connfd)
        {
            perror("error in accepting connection");
            continue;
        }

        while (1)
        {
            memset(recv_buf, 0, BUFSIZE);
            ret = recv(connfd, recv_buf, BUFSIZE, 0);
            if (0 > ret)
            {
                perror("error in receiving data");
                break;
            }
            if (0 == ret)
            {
                // Connection closed by client
                break;
            }

            recv_buf[ret] = '\0';
            if (0 == strcmp(recv_buf, "quit"))
            {
                fprintf(stderr, "server is terminated by client\n");
                break;
            }

            execute(recv_buf, connfd);
        }

        close(connfd);
    }

    fprintf(stderr, "server is down\n");
    close(listenfd);

    return 0;
}
