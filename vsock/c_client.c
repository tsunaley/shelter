#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/wait.h>

#define VSOCK_PORT 1234  // 定义VSOCK端口号

/*
 * 函数 get_argv() 用于解析用户输入的命令行参数，并将其拆分为命令和参数数组
 * 参数:
 *   - char *u_argv: 原始的用户输入命令字符串
 *   - char *user_argv[]: 用于存储解析后的参数数组
 *   - char *filename: 用于存储解析出的命令名
 *   - int* len: 用于存储解析出的参数个数
 * 返回值:
 *   - int: 正常结束返回0
 */
int get_argv(char *u_argv, char *user_argv[], char *filename, int* len) {
    char *tokens[20];
    int token_count = 0;

    // 使用 strtok 拆分字符串
    char *token = strtok(u_argv, " ");
    while (token != NULL && token_count < 20) {
        tokens[token_count] = token;
        token_count++;
        token = strtok(NULL, " ");
    }

    // 复制第一个 token 到 filename
    if (token_count > 0) {
        strcpy(filename, tokens[0]); // 直接使用 strcpy，假设 filename 大小足够
    }

    // 填充 user_argv
    for (int i = 0; i < token_count; i++) {
        user_argv[i] = tokens[i];
    }

    // 确保 user_argv 数组以 NULL 结束
    if (token_count > 1) {
        user_argv[token_count] = NULL;
    }
    *len = token_count;
    return 0;
}

/*
 * 函数 main() 是程序的入口点，负责创建VSOCK连接，解析命令行参数，创建管道与子进程通信，并通过select函数实现非阻塞的I/O操作
 * 参数:
 *   - 无
 * 返回值:
 *   - int: 正常结束返回0
 */
int main() {
    int vsock_fd, pipe_fd1[2], pipe_fd2[2];
    struct sockaddr_vm vsock_addr;
    char buffer[1024];
    char filename[1024];
    char u_argv[1024];
    char *user_argv[20];
    ssize_t read_size;


    if ((vsock_fd = socket(AF_VSOCK, SOCK_STREAM, 0)) < 0) {
        perror("vsock socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 初始化VSOCK地址结构体
    vsock_addr.svm_family = AF_VSOCK;
    vsock_addr.svm_cid = 2; 
    vsock_addr.svm_port = VSOCK_PORT;

    // 连接到VSOCK
    if (connect(vsock_fd, (struct sockaddr *)&vsock_addr, sizeof(vsock_addr)) < 0) {
        perror("vsock connect failed");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    // 从VSOCK读取原始命令行参数
    read_size = read(vsock_fd, u_argv, sizeof(u_argv));
                if (read_size <= 0) {
                    if (read_size < 0) perror("pipe read error");
                }
    printf("a to c: %s\n", u_argv);
    int len = 0;
    get_argv(u_argv, user_argv, filename, &len);

    printf("py_path: %s\n", filename);
    for(int k = 0; k < len; k++) {
        printf("arg[%d]:%s\n", k,user_argv[k]);
    }
  
    // 创建两个管道，用于父子进程通信
    if (pipe(pipe_fd1) == -1 || pipe(pipe_fd2) == -1) {
        perror("pipe failed");
        close(vsock_fd);
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        close(vsock_fd);
        close(pipe_fd1[0]);
        close(pipe_fd1[1]);
        close(pipe_fd2[0]);
        close(pipe_fd2[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
      
        close(pipe_fd1[1]);  // 关闭不需要的写端
        close(pipe_fd2[0]);  // 关闭不需要的读端

        dup2(pipe_fd1[0], STDIN_FILENO);  // 将标准输入重定向到pipe_fd1的读端
        close(pipe_fd1[0]);
  
        dup2(pipe_fd2[1], STDOUT_FILENO);   // 将标准输出重定向到pipe_fd2的写端
        close(pipe_fd2[1]);
        
        execv(filename, user_argv);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    } else {
    
        close(pipe_fd1[0]);  // 关闭不需要的读端
        close(pipe_fd2[1]);  // 关闭不需要的写端

        fd_set readfds;
        int max_fd = vsock_fd > pipe_fd2[0] ? vsock_fd : pipe_fd2[0];    // 计算最大文件描述符

        while (1) {
            FD_ZERO(&readfds);
            FD_SET(vsock_fd, &readfds);
            FD_SET(pipe_fd2[0], &readfds);

            int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
            if (activity < 0 && errno != EINTR) {
                perror("select error");
                break;
            }

            if (FD_ISSET(pipe_fd2[0], &readfds)) {
          
                read_size = read(pipe_fd2[0], buffer, sizeof(buffer));
                if (read_size <= 0) {
                    if (read_size < 0) perror("pipe read error");
                    break;
                }
                buffer[read_size] = '\0';
                write(vsock_fd, buffer, read_size);
            }
        
            if (FD_ISSET(vsock_fd, &readfds)) {
                read_size = read(vsock_fd, buffer, sizeof(buffer));
                if (read_size <= 0) {
                    if (read_size < 0) perror("vsock read error");
                    break;
                }
                buffer[read_size] = '\0';

                write(pipe_fd1[1], buffer, read_size);
            }
        }

        usleep(10000); // 延迟 10 毫秒 防止和上个信息一起发送

        char end_signal[] = "exit_lx_yxk";
        write(vsock_fd, end_signal, strlen(end_signal));

        close(vsock_fd);
        close(pipe_fd1[1]);
        close(pipe_fd2[0]);

        wait(NULL);
 
    }

    return 0;
}