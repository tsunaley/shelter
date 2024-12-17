#include "run.h"

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/*
 * 函数 exec_user_prog() 用于执行用户指定的敏感程序
 * 参数:
 *   - const char *tag: 敏感程序的标签
 *   - const char *user_command: 用户要执行的命令
 *   - const char *ip: 用于远程认证通信的IP地址
 *   - const char *mem: 机密虚拟机的内存大小
 * 返回值:
 *   - void
 */
void exec_user_prog(const char *tag, const char *user_command, const char *ip, const char *mem){
    // 将 initrd.img 复制到 Docker 容器中
    char command[1024];
    snprintf(command, sizeof(command), "sudo docker cp build/%s/initrd.img container_shelter:/usr/local/bin/initrd.img", tag);
    if (system(command) == -1) {
        LOG_ERROR("Failed to copy initrd.img to Docker container.");
    }

    // 创建子进程以启动 vsock 服务器
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // 子进程
        char command_copy[1024];
        strncpy(command_copy, user_command, sizeof(command_copy));
        command_copy[sizeof(command_copy) - 1] = '\0';
        vsock_server(command_copy);
        exit(EXIT_SUCCESS);
    } else {
        // 父进程在 Docker 容器中执行启动脚本
        char full_command[256];
        snprintf(full_command, sizeof(full_command), "docker exec container_shelter /usr/local/bin/start.sh %s %s", ip, mem);
        if (system(full_command) == -1) {
            LOG_ERROR("Failed to execute command: %s", full_command);
        }


        wait(NULL);
    }
}
