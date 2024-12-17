#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "vsock_server.h"
#include "log.h"

volatile int running = 1;  // 全局变量，控制线程的运行状态
char end_signal[] = "exit_lx_yxk";
pthread_t receive_thread, send_thread;

/*
 * 函数 receive_messages() 用于接收来自vsock host进程的信息并输出
 * 参数:
 *   - void *socket: 指向套接字的指针
 * 返回值:
 *   - void *: 返回NULL
 */
void *receive_messages(void *socket) {
    int new_socket = *(int *)socket;
    char buffer[1024];
    while (1) {
        int valread = read(new_socket, buffer, 1024);
        buffer[valread] = '\0';  
        if (strcmp(buffer, end_signal) == 0) {
        
            pthread_cancel(send_thread);
            pthread_exit(EXIT_SUCCESS);
        }

        printf("%s", buffer);
    }
    return NULL;
}

/*
 * 函数 send_messages() 用于通过套接字发送信息给vsock host进程
 * 参数:
 *   - void *socket: 指向套接字的指针
 * 返回值:
 *   - void *: 返回NULL
 */
void *send_messages(void *socket) {
    int new_socket = *(int *)socket;
    char message[1024];
    while (fgets(message, sizeof(message), stdin)) {
        
        send(new_socket, message, strlen(message), 0);
    }
    return NULL;
}

/*
 * 函数 vsock_server() 用于启动安全通信模块的宿主机进程
 * 参数:
 *   - const char *command: 用户指定敏感程序的启动参数
 * 返回值:
 *   - int: 正常结束返回0
 */
int vsock_server(const char *command) {  
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t receive_thread, send_thread;

    //  创建套接字  
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOG_ERROR("Socket creation failed");
    }

    // 初始化地址结构体
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_fd);
        LOG_ERROR("Bind failed");
    }

    // 监听绑定socket
    if (listen(server_fd, 3) < 0) {
        close(server_fd);
        LOG_ERROR("Listen failed");
    }

    //  接收socket信息
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        close(server_fd);
        LOG_ERROR("Accept failed");
    }

    //  发送用户指定敏感程序的启动参数，用于vsock guest程序启动敏感程序
    send(new_socket, command, strlen(command), 0);

    //  创建线程，用于传输和接收
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&new_socket);
    pthread_create(&send_thread, NULL, send_messages, (void *)&new_socket);

    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    close(new_socket);
    close(server_fd);
    fflush(stdout);

    return 0;
}
