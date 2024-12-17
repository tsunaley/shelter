#include "vm_manager.h"
#include "log.h"
#include "tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * 函数 make_dockerfile() 用于创建 Dockerfile
 * 参数:
 *   - const char *tag: Docker 镜像的标签
 * 返回值:
 *   - int: 正常结束返回0
 */
int make_dockerfile(const char *tag){
    char path[512];

    char *image = IMAGE_DIR;
    char *ovmf = OVMF_DIR;
    char *vsock = VSOCK_SERVER_DIR;
    char *qemu = QEMU_START_DIR;
    char *sevctl = SEVCTL_DIR;
    char *server = CERTC_HAIN;
    char *server_con = RA;

    snprintf(path, sizeof(path), "build/%s", tag);
    create_directory(path);
    
    
    char dockerfile[512];
    snprintf(dockerfile, sizeof(dockerfile), "%s/Dockerfile", path);

    char content[2048];
    snprintf(content, sizeof(content),
             "FROM ubuntu:latest\n"
             "RUN apt-get update && apt-get install -y \\\n"
             "    qemu-system-x86 \\\n"
             "    socat \\\n"
             "    && apt-get clean \n"
             "COPY %s /usr/local/bin/sevctl\n"
             "COPY %s /usr/local/bin/bzImage\n"
             "COPY %s /usr/local/bin/OVMF.fd\n"
             "COPY %s /usr/local/bin/start.sh\n"
             "COPY %s /usr/local/bin/b_relay\n"
             "COPY %s /usr/local/bin/\n"
             "COPY %s /usr/local/bin/\n"
             "RUN chmod +x /usr/local/bin/start.sh\n"
             "RUN mkdir -p /etc/qemu\n"
             "RUN echo \"allow br0\" | tee /etc/qemu/bridge.conf\n",
             sevctl, image, ovmf, qemu, vsock, server, server_con);
    
    write_to_file(dockerfile, content); // 将内容写入 Dockerfile
    return 0;
}

/*
 * 函数 image_built() 用于构建 Docker 镜像
 * 参数:
 *   - const char *tag: Docker 镜像的标签
 * 返回值:
 *   - int: 正常结束返回0
 */
int image_built(const char *tag){
    char docker_command[256];
    snprintf(docker_command, sizeof(docker_command), "sudo docker build -f build/%s/Dockerfile -t %s .", tag, tag);
    int result = system(docker_command);

    if (result != 0) {
        LOG_ERROR("Failed to build Docker image with tag: %s\n", tag);
    }

    printf("Docker image build with tag: %s\n", tag);

    return 0;
}

/* 
 * 全局变量 shelter_id 用于存储 Docker 容器的 ID
 */
char shelter_id[65] = "";


/*
 * 函数 run_container() 用于启动 Docker 容器
 * 参数:
 *   - const char *tag: Docker 镜像的标签
 * 返回值:
 *   - int: 正常结束返回0
 */
int run_container(const char *tag){
    // 创建 Docker 命令
    char docker_command[256];

    snprintf(docker_command, sizeof(docker_command), 
        "sudo docker run -e HOST_IP=$(hostname -I | awk '{print $1}') --device /dev/kvm --device /dev/sev --privileged=true -d -it --network host --name container_%s %s", tag, tag);
    
    FILE *fp = popen(docker_command, "r");
    if (fp == NULL) {
        LOG_ERROR("Failed to run Docker command.\n");
    }

    // 读取容器 ID
    if (fgets(shelter_id, sizeof(shelter_id), fp) != NULL) {
        // 去掉换行符
        shelter_id[strcspn(shelter_id, "\n")] = 0;
    } else {
        pclose(fp);
        LOG_ERROR("Failed to retrieve Docker container ID for tag: %s\n", tag);
    }

    // 关闭命令管道
    pclose(fp);
    printf("Docker container started with tag: %s\n", tag);
    return 0;
}

/*
 * 函数 shelter_init() 初始化 Docker 环境
 * 参数:
 *   - const char *tag: Docker 镜像的标签
 * 返回值:
 *   - void
 */
void shelter_init(const char *tag){
    make_dockerfile(tag);
    image_built(tag);
    run_container(tag);
}

/*
 * 函数 check_shelter_container() 检查 Docker 容器是否存在
 * 返回值:
 *   - char*: Docker 容器的 ID，如果不存在则为空字符串
 */
char *check_shelter_container() {
    // 创建检查容器的命令
    char docker_command[] = "sudo docker ps -aqf name=container_shelter";

    FILE *fp = popen(docker_command, "r");
    if (fp == NULL) {
        LOG_ERROR("Failed to run Docker command to check container.\n");
    }

    // 读取容器 ID
    if (fgets(shelter_id, sizeof(shelter_id), fp) != NULL) {
        // 去掉换行符
        shelter_id[strcspn(shelter_id, "\n")] = 0;
    }

    // 关闭命令管道
    pclose(fp);
    return shelter_id;
}

/*
 * 函数 start_container() 启动已存在的 Docker 容器
 * 参数:
 *   - const char *container_id: Docker 容器的 ID
 * 返回值:
 *   - void
 */
void start_container(const char *container_id) {
    char docker_command[256];
    snprintf(docker_command, sizeof(docker_command), "sudo docker inspect -f '{{.State.Running}}' %s", container_id);
    
    FILE *fp = popen(docker_command, "r");
    if (fp == NULL) {
        LOG_ERROR("Failed to run Docker command to inspect container.\n");
        return; 
    }
    char running_status[16];
    if (fgets(running_status, sizeof(running_status), fp) != NULL) {
        running_status[strcspn(running_status, "\n")] = 0;
        if (strcmp(running_status, "false") == 0) {
            // 容器未运行，启动容器
            snprintf(docker_command, sizeof(docker_command), "sudo docker start %s", container_id);
            system(docker_command);
            printf("Started the existing container with ID: %s\n", container_id);
        }
    }
    pclose(fp);
}
