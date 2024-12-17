#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  // for open and O_RDWR

#define PORT 8121
#define BUFFER_SIZE 4096

void execute_command(const char *command) {
    int status = system(command);
    if (status == -1) {
        perror("system");
        exit(EXIT_FAILURE);
    }
}

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
    // Send end of file signal
    send(socket, "EOF", 3, 0);
}

void receive_file(int socket, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_received;
    
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        // Check if the buffer contains the EOF signal
        // Note: Make sure that the "EOF" signal is not split across multiple packets
        if (bytes_received >= 3 && strncmp(buffer + bytes_received - 3, "EOF", 3) == 0) {
            // Write only the part of the buffer before "EOF"
            fwrite(buffer, 1, bytes_received - 3, file);
            break;
        } else {
            // Write the entire buffer to file
            fwrite(buffer, 1, bytes_received, file);
        }
    }
    
    fclose(file);
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    char signalbuffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    //设置socket选项以立即重用端口
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    { 
        perror("setsockopt"); 
        close(server_fd); 
        exit(EXIT_FAILURE); 
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("bind succeed\n");
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("listen succeed\n");

    // 接受客户端连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n connect succeed\n");

    // Step 0: 导出证书链：sudo /home/a/.cargo/bin/sevctl export /tmp/sev.pdh
    // Step 1: 将 sev.pdh 文件发送到客户端
    send_file(new_socket, "/tmp/sev.pdh");
    printf("Send sev pdh succeed\n");

    shutdown(new_socket, SHUT_WR);  // Notify client that server finished sending

    // Step 2: 接收客户端发来的sev_godh.b64和sev_session.b64文件
    receive_file(new_socket, "sev_godh.b64");

    // 关闭当前连接
    close(new_socket);

    // 重新监听端口并接受新连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    receive_file(new_socket, "sev_session.b64");

    printf("Receive sev_godh.b64 and sev_session.b64 succeed\n");

    close(new_socket);
    close(server_fd);
    return 0;
}