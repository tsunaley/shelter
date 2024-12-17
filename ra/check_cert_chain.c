#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>  // for open and O_RDWR

#define PORT 8121
#define BUFFER_SIZE 4096

/*
 * 函数 execute_command() 用于执行命令
 * 参数:
 *   - const char *command: 待执行的命令
 * 返回值:
 *   无
 */
void execute_command(const char *command) {
    int status = system(command);
    if (status == -1) {
        perror("system");
        exit(EXIT_FAILURE);
    }
}

/*
 * 函数 send_file() 用于向客户端发送文件
 * 参数:
 *   - int socket: 套接字
 *   - const char *filename：文件名称
 * 返回值:
 *   无
 */
void send_file(int socket, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(socket, buffer, bytes_read, 0);
    }
    fclose(file);
    send(socket, "EOF", 3, 0);
}

/*
 * 函数 receive_file() 用于从客户端接收文件
 * 参数:
 *   - int socket:  套接字
 *   - const char *filename：文件名称
 * 返回值:
 *   无
 */
void receive_file(int socket, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_received;
    
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        if (bytes_received >= 3 && strncmp(buffer + bytes_received - 3, "EOF", 3) == 0) {
            fwrite(buffer, 1, bytes_received - 3, file);
            break;
        } else {
            fwrite(buffer, 1, bytes_received, file);
        }
    }
    
    fclose(file);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return -1;
    }
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char measurement[BUFFER_SIZE];
    const char *server_ip = argv[1];

    // 创建套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 转换IPv4和IPv6地址从文本到二进制形式
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // 连接服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    printf("\nConnection Succeed\n");
    
    // Step 0: 导出证书链
    // Step 1: 将 sev.pdh 文件发送到客户端
    send_file(sock, "/tmp/sev.pdh");
    printf("Send sev pdh succeed\n");

    shutdown(sock, SHUT_WR);  

    // Step 2: 接收客户端发来的sev_godh.b64和sev_session.b64文件
    receive_file(sock, "sev_godh.b64");

    close(sock);
    usleep(100); 

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    receive_file(sock, "sev_session.b64");
    printf("Receive sev_godh.b64 and sev_session.b64 succeed\n");

    shutdown(sock, SHUT_WR);  
    close(sock);

}
