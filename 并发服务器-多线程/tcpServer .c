#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8900
#define BUFSIZE 2048

// 定义一个结构体来存放线程参数
typedef struct {
    int connfd;
} thread_param;

// 执行命令并将其结果发送回客户端
int execute(char* cmd, int sockfd) {
    char buf[BUFSIZE];
    FILE* fp;
    char errbuf[BUFSIZE] = "command cannot execute\n"; 
    int ret;
    int counter = 0;

    memset(buf, 0, BUFSIZE);

    // 打开管道执行命令
    if ((fp = popen(cmd, "r")) == NULL) { 
        perror("popen failed");
        ret = send(sockfd, errbuf, strlen(errbuf), 0);
        return -1;
    }

    // 读取命令的输出
    while ((counter < BUFSIZE - 1) && (fgets(buf + counter, BUFSIZE - counter, fp) != NULL)) {
        counter = strlen(buf);
    }

    // 如果没有输出，则发送指令无输出的信息
    if (counter == 0) {
        snprintf(errbuf, BUFSIZE, "指令无输出: %s\n", cmd);
        send(sockfd, errbuf, strlen(errbuf), 0);
        pclose(fp);
        return -1;
    }

    ret = send(sockfd, buf, counter, 0);
    if (ret < 0) {
        return -1;
    }

    pclose(fp);
    return 0;
}


// 线程函数
void* handle_client(void* arg) {
    thread_param* param = (thread_param*)arg;
    int connfd = param->connfd;
    free(param); // 释放传入的参数

    char send_buf[BUFSIZE], recv_buf[BUFSIZE];
    int ret;

    while (1) {
        memset(recv_buf, 0, BUFSIZE);
        ret = recv(connfd, recv_buf, BUFSIZE, 0);
        if (0 > ret) {
            perror("error in receiving data");
            break;
        }
        if (0 == ret) {
            // Connection closed by client
            break;
        }

        recv_buf[ret] = '\0';
        if (0 == strcmp(recv_buf, "quit")) {
            fprintf(stderr, "server is terminated by client\n");
            break;
        }

        execute(recv_buf, connfd);
    }

    close(connfd);
    return NULL;
}

int main(int argc, char** argv) {
    // declare variable
    int listenfd;
    struct sockaddr_in serv, client;
    int opt = 1;
    socklen_t len = sizeof(struct sockaddr);
    pthread_t tid;

    // create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > listenfd) {
        perror("error in creating socket");
        exit(-1);
    }

    // set the socket (allow re-bind)
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    // set the value for serv
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT);

    // bind the socket
    if (bind(listenfd, (struct sockaddr*)&serv, sizeof(struct sockaddr)) < 0) {
        perror("error in binding");
        exit(-1);
    }

    // listen for incoming connections
    if (listen(listenfd, 5) < 0) {
        perror("error in listening");
        exit(-1);
    }

    // accept incoming connection and handle communication
    while (1) {
        int connfd = accept(listenfd, (struct sockaddr*)&client, &len);
        if (0 > connfd) {
            perror("error in accepting connection");
            continue;
        }

        // 创建新的线程来处理客户端
        thread_param* param = malloc(sizeof(thread_param));
        if (param == NULL) {
            perror("Failed to allocate memory for thread parameters");
            close(connfd);
            continue;
        }
        
        param->connfd = connfd; // 将连接的套接字传递给线程
        if (pthread_create(&tid, NULL, handle_client, param) != 0) {
            perror("Failed to create thread");
            free(param); // 线程创建失败时释放参数
            close(connfd);
        }

        pthread_detach(tid); // 将线程设置为分离模式，避免内存泄漏
    }

    fprintf(stderr, "server is down\n");
    close(listenfd);

    return 0;
}