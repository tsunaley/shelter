#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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
    } else {
        printf("Command executed successfully with exit status: %d\n", WEXITSTATUS(status));
    }
}

/*
 * 函数 send_file() 用于向服务端发送文件
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
 * 函数 receive_file() 用于从服务端接收文件
 * 参数:
 *   - int socket: 套接字
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

int main(){
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

    // Step 1: 接收服务器发送的 sev.pdh 文件
    receive_file(new_socket, "sev.pdh");
    printf("Receive sev pdh succeed\n");

    // Step 2: 验证文件完整性
    snprintf(command, sizeof(command), "sevctl verify --sev sev.pdh");
    execute_command(command);

    // Step 3: 执行 sevctl session 命令
    snprintf(command, sizeof(command), "sevctl session -n 'sev' sev.pdh 1");
    execute_command(command);
    
    // Step 4: 将 sev_godh.b64 和 sev_session.b64 文件发送到服务器
    send_file(new_socket, "sev_godh.b64");  

    // 关闭当前连接
    close(new_socket);

    // 重新监听端口并接受新连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    send_file(new_socket, "sev_session.b64");

    shutdown(new_socket, SHUT_WR);
    close(new_socket);
    close(server_fd);
    return 0;
}
