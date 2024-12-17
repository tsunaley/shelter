#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  

#define PORT 8121
#define BUFFER_SIZE 4096
#define COMMAND_BUFFER_SIZE 8192  // 增加command缓冲区大小

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
 * 函数 execute_command_and_capture_output() 用于执行命令并读取输出
 * 参数:
 *   - const char *command: 待执行的命令
 *   - const char *output: 读取执行命令后的输出
 *   - const char *output_size: 读取执行命令后的输出的大小
 * 返回值:
 *   无
 */
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
    const char *server_ip = argv[1];
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char command[COMMAND_BUFFER_SIZE];
    char measurement[BUFFER_SIZE];
    char result[BUFFER_SIZE];

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

    // Step1：发送包含CVM启动度量值的measurement.bin文件
    send_file(sock, "measurement.bin");
    printf("Sent SEV measurement to client\n");
    shutdown(sock, SHUT_WR);

    receive_file(sock, "secret_header.bin");

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

    // Step2：从客户端接收secret_payload.bin文件
    receive_file(sock, "secret_payload.bin");

    snprintf(command, sizeof(command),"base64 -w 0 < secret_header.bin");
    execute_command_and_capture_output(command, buffer, sizeof(buffer));
    // 删除最后一个换行符，如果有的话
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    // Step2：将结果写入文件secret_header.b64
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
    
    // Step3：将结果写入文件secret_payload.b64
    file = fopen("secret_payload.b64", "w");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", buffer);
    fclose(file);

    close(sock);

}
