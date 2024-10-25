#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVERPORT 1573
#define BACKLOG 10
#define BUFSIZE 2048

struct client_info {
    int client_id;
    struct sockaddr_in client_address;
    char name[256];
    int active;
};

void broadcast_message(struct client_info* clients, int max_fd, int sender_fd, char* message) {
    for (int i = 0; i <= max_fd; i++) {
        if (clients[i].active && i != sender_fd) {
            send(i, message, strlen(message), 0);
        }
    }
}

void list_active_clients(struct client_info* clients, int max_fd, int requester_fd) {
    char buffer[BUFSIZE] = "Active clients:\n";
    for (int i = 0; i <= max_fd; i++) {
        if (clients[i].active) {
            strcat(buffer, clients[i].name);
            strcat(buffer, "\n");
        }
    }
    send(requester_fd, buffer, strlen(buffer), 0);
}

int main() {
    fd_set master_fds, read_fds;
    struct sockaddr_in server_addr, client_addr;
    int max_fd, sockfd, newfd, nbytes, opt = 1, addr_len = sizeof(struct sockaddr);
    char data_buf[BUFSIZE], send_buf[BUFSIZE];
    struct client_info clients[FD_SETSIZE];

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    memset(clients, 0, sizeof(clients));

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt() error");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind() error");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen() error");
        exit(1);
    }

    FD_SET(sockfd, &master_fds);
    max_fd = sockfd;
    printf("server is ok!\n");

    while (1) {
        read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select() error");
            exit(1);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                    newfd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);
                    if (newfd == -1) {
                        perror("accept() error");
                    }
                    else {
                        FD_SET(newfd, &master_fds);
                        if (newfd > max_fd) {
                            max_fd = newfd;
                        }
                        clients[newfd].client_id = newfd;
                        clients[newfd].client_address = client_addr;
                        clients[newfd].active = 1;
                        printf("New connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), newfd);
                    }
                }
                else {
                    nbytes = recv(i, data_buf, sizeof(data_buf), 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("socket %d hung up\n", i);
                        }
                        else {
                            perror("recv() error");
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                        clients[i].active = 0;
                    }
                    else {
                        data_buf[nbytes] = '\0';
                        if (clients[i].name[0] == '\0') {
                            strncpy(clients[i].name, data_buf, sizeof(clients[i].name) - 1);
                            clients[i].name[strcspn(clients[i].name, "\n")] = '\0'; // Remove newline character
                            send(i, "client ok\n", 10, 0);
                        }
                        else if (data_buf[0] == '/') {
                            if (strncmp(data_buf, "/help", 5) == 0) {
                                send(i, "/help: show this help message\n/quit: quit the chat room\n/who: show active users\n/send <username> <message>: send a private message\n", 128, 0);
                            }
                            else if (strncmp(data_buf, "/quit", 5) == 0) {
                                printf("client %s quit\n", clients[i].name);
                                snprintf(send_buf, sizeof(send_buf), "%s has left the chat\n", clients[i].name);
                                broadcast_message(clients, max_fd, i, send_buf);
                                close(i);
                                FD_CLR(i, &master_fds);
                                clients[i].active = 0;
                            }
                            else if (strncmp(data_buf, "/who", 4) == 0) {
                                list_active_clients(clients, max_fd, i);
                            }
                            else if (strncmp(data_buf, "/send", 5) == 0) {
                                char* token = strtok(data_buf, " ");
                                token = strtok(NULL, " ");
                                char* target_user = token;
                                token = strtok(NULL, "");
                                char* private_message = token;
                                int target_fd = -1;
                                for (int j = 0; j <= max_fd; j++) {
                                    if (clients[j].active && strcmp(clients[j].name, target_user) == 0) {
                                        target_fd = j;
                                        break;
                                    }
                                }
                                if (target_fd != -1) {
                                    snprintf(send_buf, sizeof(send_buf), "Private message from %s: %s\n", clients[i].name, private_message);
                                    send(target_fd, send_buf, strlen(send_buf), 0);
                                }
                                else {
                                    send(i, "User not found\n", 16, 0);
                                }
                            }
                        }
                        else {
                            snprintf(send_buf, sizeof(send_buf), "%s: %s", clients[i].name, data_buf);
                            broadcast_message(clients, max_fd, i, send_buf);
                        }
                    }
                }
            }
        }
    }

    return 0;
}