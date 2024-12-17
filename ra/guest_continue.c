#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8121
#define BUFFER_SIZE 4096
#define COMMAND_BUFFER_SIZE 8192  

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
 * 函数 execute_command_and_capture_output() 用于执行命令并读取输出
 * 参数:
 *   - const char *command: 待执行的命令
 *   - const char *output: 读取执行命令后的输出
 *   - const char *output_size: 读取执行命令后的输出的大小
 * 返回值:
 *   无
 */
void execute_command_and_capture_output(const char *command, char *output, size_t output_size) {
    FILE *fp;
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }
    
    // 读取输出
    size_t len = 0;
    while (fgets(output + len, output_size - len, fp) != NULL) {
        len += strlen(output + len);
        if (len >= output_size - 1) {
            break;
        }
    }
    
    pclose(fp);
}

/*
 * 函数 extract_measurement() 提取度量值
 * 参数:
 *   - const char *filename: 文件名称
 *   - const char *buffer: 存储度量值的数组
 *   - const char *buffer_size: 存储度量值数组的大小
 * 返回值:
 *   无
 */
void extract_measurement(const char *filename, char *buffer, size_t buffer_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // 将整个文件目录读取到一个临时的数组里
    char temp_buffer[BUFFER_SIZE * 4]; 
    size_t bytes_read = fread(temp_buffer, 1, sizeof(temp_buffer), file);
    if (bytes_read <= 0) {
        perror("fread");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    temp_buffer[bytes_read] = '\0'; 
    fclose(file);

    // 搜素度量值
    const char *start_marker = "\"data\": \"";
    char *start = strstr(temp_buffer, start_marker);
    if (start) {
        start += strlen(start_marker);
        char *end = strchr(start, '"');
        if (end) {
            size_t length = end - start;
            if (length < buffer_size) {
                strncpy(buffer, start, length);
                buffer[length] = '\0'; 
            } else {
                fprintf(stderr, "Buffer size is too small to hold the measurement string\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "End quote not found after data\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Start marker not found in the file\n");
        exit(EXIT_FAILURE);
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
        if (strncmp(buffer, "EOF", 3) == 0) {
            break;
        }
        fwrite(buffer, 1, bytes_received, file);
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
    FILE *qmp_socket;
    
    char result[BUFFER_SIZE];


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

    // Step1：接收度量值并存到buffer里
    receive_file(new_socket, "measurement.bin");
    extract_measurement("measurement.bin", buffer, sizeof(buffer));
    printf("Received SEV measurement from server\n");
    printf("Measurement output:\n%s %ld\n", buffer,strlen(buffer));

    // Step 2: 使用接收到的度量值构造 sevctl 命令
    snprintf(command, sizeof(command),
             "sevctl measurement build --api-major 0 --api-minor 17 --build-id 48 --policy 0x01 --tik sev_tik.bin --launch-measure-blob %s --firmware ../default_component/OVMF.fd",
             buffer);

    // Step 3: 执行 sevctl 命令并捕获输出
    execute_command_and_capture_output(command, result, sizeof(result));
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == '\n') {
        result[len - 1] = '\0';  // 删除最后一个换行符
    }
    printf("Command output:\n%s %ld\n", result,strlen(result));


    // 检查获取到的度量值与预期度量值是否一致
    if (strcmp(buffer, result) == 0) {
        // Step 4: 执行 echo 命令来写入秘密
        execute_command("echo \"TOP_SECRET_MESSAGE\" > ./secret.txt");
        printf("Secret message written to ./secret.txt\n");

        // Step 5: 使用 sevctl 命令构造秘密
        snprintf(command, sizeof(command),
                 "sevctl secret build --tik sev_tik.bin --tek sev_tek.bin --launch-measure-blob %s --secret 43ced044-42ec-487a-88b7-261bda359f24:./secret.txt ./secret_header.bin ./secret_payload.bin",
                 buffer);
        execute_command(command);
        printf("Secret built successfully\n");
        usleep(10); 

        // Step 6: 将 sev_godh.b64 和 sev_session.b64 文件发送到服务器
        send_file(new_socket, "secret_header.bin");
    
        // 关闭连接
        shutdown(new_socket, SHUT_WR);  
        close(new_socket);

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    
       send_file(new_socket, "secret_payload.bin");

       shutdown(new_socket, SHUT_WR);  

    } else {
       printf("Buffer and result do not match\n");
    }
    close(new_socket);
    close(server_fd);
    return 0;
}