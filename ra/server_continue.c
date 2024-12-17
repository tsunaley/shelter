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

void execute_command_and_capture_output(const char* command, char* result, size_t result_size) {
    FILE* fp;
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        exit(EXIT_FAILURE);
    }

    if (fgets(result, result_size, fp) == NULL) {
        perror("fgets failed");
        exit(EXIT_FAILURE);
    }

    pclose(fp);
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
    FILE *qmp_socket;

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

    // 发送度量值到客户端
    send_file(new_socket, "measurement.bin");
    printf("Sent SEV measurement to client\n");

    shutdown(new_socket, SHUT_WR);  // Notify client that server finished sending
    
    // Step 2: 接收客户端发来的sev_godh.b64和sev_session.b64文件
    receive_file(new_socket, "secret_header.bin");

    // 关闭当前连接
    close(new_socket);

    // 重新监听端口并接受新连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    receive_file(new_socket, "secret_payload.bin");

    snprintf(command, sizeof(command),"base64 -w 0 < secret_header.bin");
    execute_command_and_capture_output(command, buffer, sizeof(buffer));
    // 删除最后一个换行符，如果有的话
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    // 将结果写入文件
    FILE* file = fopen("secret_header.b64", "w");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", buffer);
    fclose(file);

    snprintf(command, sizeof(command),"base64 -w 0 < secret_payload.bin");
    execute_command_and_capture_output(command, buffer, sizeof(buffer));
    // 删除最后一个换行符，如果有的话
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    // 将结果写入文件
    file = fopen("secret_payload.b64", "w");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", buffer);
    fclose(file);



    close(new_socket);
    close(server_fd);
    return 0;
}
