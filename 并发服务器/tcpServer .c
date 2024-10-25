#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8900
#define BUFSIZE 2048

// 使用管道运行命令
FILE* mypopen(char* cmd) {
    int f_des[2];
    pid_t pid;

    // 创建管道
    if (pipe(f_des) == -1) {
        perror("pipe failed");
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return NULL;
    }

    if (pid == 0) { // 子进程
        close(f_des[0]); // 关闭读端
        dup2(f_des[1], STDOUT_FILENO); // 重定向标准输出
        close(f_des[1]); // 关闭写端描述符

        // 整理命令参数
        char* argv[BUFSIZE / 2 + 1]; // 存储命令及其参数
        int i = 0;
        char *token = strtok(cmd, " \n");
        while (token != NULL && i < (BUFSIZE / 2)) {
            argv[i++] = token;
            token = strtok(NULL, " \n");
        }
        argv[i] = NULL; 

        execvp(argv[0], argv); // 执行命令
        perror("exec failed"); 
        _exit(1); // 使用 _exit 在子进程中退出
    } else { // 父进程
        close(f_des[1]); // 关闭写端
        return fdopen(f_des[0], "r"); // 将读端转换为文件指针
    }
}

// 执行命令
int execute(char* cmd, int sockfd) {
    char buf[BUFSIZE];
    const char* errbuf = "command cannot execute\n";
    FILE* fp;

    // 使用 mypopen 执行命令，并返回输出的文件指针
    fp = mypopen(cmd);
    if (fp == NULL) { // 确保 mypopen 成功
        perror("open failed!");
        send(sockfd, errbuf, strlen(errbuf), 0);
        return -1;
    }

    // 读取命令的输出并发送给客户端
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, sizeof(buf), fp)) > 0) {
        send(sockfd, buf, bytesRead, 0); // 将输出发送给客户端
    }

    fclose(fp); // 关闭文件指针
    return 0;
}

int main(int argc, char** argv) {
    int listenfd, connfd;
    struct sockaddr_in serv, client;
    int opt;
    socklen_t len;

    // 创建 socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("error in creating socket");
        exit(-1);
    }

    opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    // 设置服务器地址
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT);

    // 绑定 socket
    if (bind(listenfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("error in binding");
        exit(-1);
    }

    // 监听连接
    if (listen(listenfd, 5) < 0) {
        perror("error in listening");
        exit(-1);
    }

    // 接受连接并处理客户端
    while (1) {
        len = sizeof(client);
        connfd = accept(listenfd, (struct sockaddr*)&client, &len);
        if (connfd < 0) {
            perror("error in accepting connection");
            continue;
        }

        if (fork() == 0) { // 创建子进程处理客户端请求
            close(listenfd); // 子进程关闭监听套接字
            
            char recv_buf[BUFSIZE];
            while (1) {
                memset(recv_buf, 0, BUFSIZE);
                int ret = recv(connfd, recv_buf, BUFSIZE, 0);
                if (ret < 0) {
                    perror("error in receiving data");
                    break;
                }
                if (ret == 0) {
                    // 客户端关闭连接
                    break;
                }

                recv_buf[ret] = '\0'; // 添加字符串结束符

                if (strcmp(recv_buf, "quit") == 0) {
                    fprintf(stderr, "server is terminated by client\n");
                    break;
                }

                execute(recv_buf, connfd); // 执行接收到的命令
            }

            close(connfd);
            _exit(0); // 使用 _exit() 结束子进程
        }

        close(connfd); // 父进程关闭连接套接字
    }

    close(listenfd);
    return 0;
}
