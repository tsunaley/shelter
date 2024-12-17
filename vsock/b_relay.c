#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/vm_sockets.h>
#include <sys/socket.h>
#include <pthread.h>

#define A_PORT 12345         // 定义TCP端口号
#define VSOCK_PORT 1234       // 定义VSOCK端口号

int tcp_fd, vsock_fd, vsock_client_fd;  // 定义TCP和VSOCK套接字文件描述符
pthread_mutex_t lock;  // 互斥锁，用于保护共享资源

/*
 * 函数 receive_from_vsock_send_to_tcp() 用于接收来自VSOCK客户端的信息并发送到TCP套接字
 * 参数:
 *   - void *arg: 线程参数（未使用）
 * 返回值:
 *   - void *: 返回NULL
 */
void *receive_from_vsock_send_to_tcp(void *arg) {
    char buffer[1024];
    while (1) {
        int valread = read(vsock_client_fd, buffer, sizeof(buffer));
        if (valread <= 0) break;
        buffer[valread] = '\0';
        
        pthread_mutex_lock(&lock);
        send(tcp_fd, buffer, valread, 0);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}


/*
 * 函数 receive_from_tcp_send_to_vsock() 用于接收来自TCP套接字的信息并发送到VSOCK客户端
 * 参数:
 *   - void *arg: 线程参数（未使用）
 * 返回值:
 *   - void *: 返回NULL
 */
void *receive_from_tcp_send_to_vsock(void *arg) {
    char buffer[1024];
    while (1) {
        int valread = read(tcp_fd, buffer, sizeof(buffer));
        if (valread <= 0) break;
        buffer[valread] = '\0';
        
        pthread_mutex_lock(&lock);
        send(vsock_client_fd, buffer, valread, 0);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

/*
 * 函数 main() 是程序的入口点，负责初始化套接字、创建线程，并进行数据的传输和接收
 * 参数:
 *   - 无
 * 返回值:
 *   - int: 正常结束返回0
 */
int main() {
    struct sockaddr_vm vsock_addr;  // 用于VSOCK地址的结构体
    struct sockaddr_in tcp_addr;  // 用于TCP地址的结构体
    char *host_ip;  // 存储主机的IP地址
    pthread_t vsock_to_tcp_thread, tcp_to_vsock_thread;  // 用于VSOCK和TCP通信的线程

    // 获取主机IP地址
    host_ip = getenv("HOST_IP");
    if (host_ip == NULL) {
        fprintf(stderr, "Failed to get host IP address from environment\n");
        exit(EXIT_FAILURE);
    }

    // 创建TCP套接字
    if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("tcp socket creation failed");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    // 设置TCP地址结构体
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(A_PORT);
    tcp_addr.sin_addr.s_addr = inet_addr(host_ip);

    // 连接到指定的TCP端口
    if (connect(tcp_fd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("tcp connect failed");
        close(vsock_fd);
        close(tcp_fd);
        exit(EXIT_FAILURE);
    }
    
    if ((vsock_fd = socket(AF_VSOCK, SOCK_STREAM, 0)) < 0) {
        perror("vsock socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 初始化VSOCK地址结构体
    memset(&vsock_addr, 0, sizeof(vsock_addr));
    vsock_addr.svm_family = AF_VSOCK;
    vsock_addr.svm_cid = VMADDR_CID_ANY;
    vsock_addr.svm_port = VSOCK_PORT;

    // 绑定VSOCK套接字到指定端口
    if (bind(vsock_fd, (struct sockaddr*)&vsock_addr, sizeof(vsock_addr)) < 0) {
        perror("vsock bind failed");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(vsock_fd, 3) < 0) {
        perror("vsock listen");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    // 接受来自VSOCK客户端的连接
    vsock_client_fd = accept(vsock_fd, NULL, NULL);
    if (vsock_client_fd < 0) {
        perror("vsock accept failed");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);

    // 创建用于VSOCK到TCP传输和TCP到VSOCK传输的线程
    pthread_create(&vsock_to_tcp_thread, NULL, receive_from_vsock_send_to_tcp, NULL);
    pthread_create(&tcp_to_vsock_thread, NULL, receive_from_tcp_send_to_vsock, NULL);

    // 等待线程结束
    pthread_join(vsock_to_tcp_thread, NULL);
    pthread_join(tcp_to_vsock_thread, NULL);

    close(vsock_client_fd);
    close(vsock_fd);
    close(tcp_fd);
    pthread_mutex_destroy(&lock);

    return 0;
}
