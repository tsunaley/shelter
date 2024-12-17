#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "main.h"
#include "vm_manager.h"
#include "run.h"
#include "initrd_creator.h"
#include "log.h"
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
/*
 * 函数 main() 是程序的入口点，负责解析命令行参数并调用相应的功能
 * 参数:
 *   - int argc: 命令行参数的数量
 *   - char *argv[]: 命令行参数的数组
 * 返回值:
 *   - int: 程序执行的返回值，0 表示成功
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        LOG_ERROR("Usage: %s [options] <command>", argv[0]);
    }

    const char *command = argv[1];
    int opt;
    char *tag = NULL;

    if (strcmp(command, "run") == 0) {
        char *user_command = NULL;
        char *ip = NULL;
        char *mem = NULL;
        struct option long_options[] = {
            {"tag", required_argument, 0, 't'},
            {"command", required_argument, 0, 'c'},
            {"ip", required_argument, 0, 'i'},
            {"memory", required_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        while ((opt = getopt_long(argc - 1, argv + 1, "t:c:i:m:", long_options, NULL)) != -1) {
            switch (opt) {
                case 't':
                    tag = optarg;
                    break;
                case 'c':
                    user_command = optarg;
                    break;
                case 'i':
                    ip = optarg;
                    break;
                case 'm':
                    mem = optarg;
                    break;
                default:
                    LOG_ERROR("Usage: %s run -t <tag> -c <command> -i <IP address>", argv[0]);
            }
        }
        if (tag == NULL||user_command == NULL||ip ==NULL || mem==NULL) {
            LOG_ERROR("Usage: %s run -t <tag> -c <command> -i <IP address>", argv[0]);
        }
        run_command(tag, user_command, ip, mem);
    } else if (strcmp(command, "build") == 0) {
        char *file_name = NULL;
        int mode = CODE;
        char *work_path = NULL;
        char *aa_path = NULL;

        struct option long_options[] = {
            {"tag", required_argument, 0, 't'},
            {"file", required_argument, 0, 'f'},
            {"rootfs", required_argument, 0, 'r'},
            {"path", required_argument, 0, 'p'},
            {"aa", required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };

        while ((opt = getopt_long(argc - 1, argv + 1, "t:f:r:p:a:", long_options, NULL)) != -1) {
            switch (opt) {
                case 't':
                    tag = optarg;
                    break;
                case 'f':
                    file_name = optarg;
                    mode = CODE;
                    break;
                case 'r':
                    file_name = optarg;
                    mode = ROOTFS;
                    break;
                case 'p':
                    work_path = optarg;
                    break;
                case 'a':
                    aa_path = optarg;
                    break;
                default:
                    LOG_ERROR("Usage: %s build -t <tag> -f/-r <file_name>", argv[0]);
            }
        }

        if (tag == NULL || file_name == NULL || (mode == ROOTFS && work_path == NULL)) {
            LOG_ERROR("Usage: %s build -t <tag> -f <file_name>", argv[0]);
        }
        build_command(tag, file_name, mode, work_path, aa_path);
    } 
     else if (strcmp(command, "help") == 0) {
        print_usage();
    }
    else if(strcmp(command, "delete") == 0){
        struct option long_options[] = {
            {"tag", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        while ((opt = getopt_long(argc - 1, argv + 1, "t:", long_options, NULL)) != -1) {
            switch (opt) {
                case 't':
                    tag = optarg;
                    break;
                default:
                    LOG_ERROR("Usage: %s delete -t <tag>", argv[0]);
            }
        }
        if (tag == NULL) {
            LOG_ERROR("Usage: %s delete -t <tag>", argv[0]);
        }
        
        char path[512] = "";
        snprintf(path, sizeof(path), "build/%s", tag);
        if(access(path, 0)==-1){
            printf("Tag: %s has not exist.\n", tag);
            return 0;
        }
        delete_command(path);
    } 
    else if(strcmp(command, "attestation") == 0){
        attestation_command();
    }
    else {
        LOG_ERROR("Unknown command: %s\nUsage: shelter [options] <command>\nUse: shelter help for more information.", command);
    }

    return 0;
}

/*
 * 函数 print_usage() 打印程序的使用说明
 * 参数:
 *   - void
 * 返回值:
 *   - void
 */
void print_usage() {
    printf("Usage: shelter [options] <command>\n");
    printf("Commands:\n");
    
    // attestation 指令
    printf("%-12s %s\n", "attestation", "Run remote attestation.");

    // build 指令
    printf("%-12s %s\n", "build", "Build a CVM image");
    printf("  Options for build:\n");
    printf("  %-10s %s\n", "-t", "Specify the tag for the sensitive program (each program has a unique tag)");
    printf("  %-10s %s\n", "-f", "Specify the path to the sensitive program");
    printf("  %-10s %s\n", "-r", "Specify the path to the root filesystem");
    printf("  %-10s %s\n", "-a", "Specify the path to the attestation agent");
    printf("  %-10s %s\n", "-p", "Specify the path to the work path");

    // delete 指令
    printf("%-12s %s\n", "delete", "Delete a CVM image.");
    printf("  Options for delete:\n");
    printf("  %-10s %s\n", "-t", "Specify the tag for the sensitive program (each program has a unique tag)");
    
    // help 指令
    printf("%-12s %s\n", "help", "Display specific types of command line options.");

    // run 指令
    printf("%-12s %s\n", "run", "Run a sensitive program");
    printf("  Options for run:\n");
    printf("  %-10s %s\n", "-t", "Specify the tag for the sensitive program (each program has a unique tag)");
    printf("  %-10s %s\n", "-c", "Specify the command to execute when running the sensitive program");
    printf("  %-10s %s\n", "-m", "Specify the memory size for the confidential virtual machine");
    printf("  %-10s %s\n", "-i", "Specify the IP address for remote attestation communication");


}


/*
 * 函数 run_command() 执行 run 命令
 * 参数:
 *   - const char *tag: 敏感程序的标签
 *   - const char *command: 要执行的命令
 *   - const char *ip: 用于远程认证通信的IP地址
 *   - const char *mem: 机密虚拟机的内存大小
 * 返回值:
 *   - void
 */
void run_command(const char *tag, const char *command, const char *ip, const char *mem) {
    char* container_id = check_shelter_container();
    if(strlen(container_id) == 0){
        shelter_init("shelter");
    }
    else{
        start_container(container_id);
    }
    char path[512] = "";
    snprintf(path, sizeof(path), "build/%s", tag);
    if(access(path, 0)==-1){
        printf("Tag: %s has not built.\n", tag);
        return;
    }
    exec_user_prog(tag, command, ip, mem);
}

/*
 * 函数 build_command() 执行 build 命令
 * 参数:
 *   - const char *tag: 敏感程序的标签
 *   - const char *file_name: 要打包的文件名
 *   - int mode: 模式（CODE 或 ROOTFS）
 * 返回值:
 *   - void
 */
void build_command(const char *tag, const char *file_name, int mode, const char *work_path, const char *aa_path) {
    char* container_id = check_shelter_container();

    // 若不存在shelter镜像则构建一个
    if(strlen(container_id) == 0){
        shelter_init("shelter");
    }
    char path[512] = "";
    snprintf(path, sizeof(path), "build/%s", tag);
    if(access(path, 0)!=-1){
        printf("Tag: %s has already built.\n", tag);
        return;
    }
    initrd_create(tag, file_name, mode, work_path, aa_path);
}


/*
 * 函数 delete_command() 执行 delete 命令
 * 参数:
 *   - const char *path: 要删除的路径
 * 返回值:
 *   - void
 */

void delete_command(const char *path) {
    // printf("%s\n", path);
    struct stat dir_stat;
    DIR *dirp;
    struct dirent *dp;
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[512];

    // Use lstat to handle symbolic links properly
    if ( 0 > lstat(path, &dir_stat) ) {
        LOG_ERROR("get directory stat error");
        return;
    }

    if ( S_ISREG(dir_stat.st_mode) ) {  // Regular file
        remove(path);
    } else if ( S_ISDIR(dir_stat.st_mode) ) {   // Directory file
        dirp = opendir(path);
        if (dirp == NULL) {
            LOG_ERROR("Failed to open directory");
            return;
        }

        while ( (dp = readdir(dirp)) != NULL ) {
            // Ignore . and ..
            if ( (0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
                continue;
            }

            sprintf(dir_name, "%s/%s", path, dp->d_name);
            delete_command(dir_name);   // Recursive call
        }
        closedir(dirp);

        rmdir(path);     // Remove empty directory
    } else if ( S_ISLNK(dir_stat.st_mode) ) {   // Handle symbolic links
        remove(path);
    } else {
        LOG_ERROR("Unknown file type!");
    }
}


/*
 * 函数 attestation_command() 执行 attestation 命令
 * 返回值:
 *   - void
 */
void attestation_command(){
    // 进入 grpc_guest 目录
    if (chdir("grpc_guest") != 0) {
        LOG_ERROR("chdir failed");
        return ;
    }

    // 执行 cargo run 指令
    int result = system("cargo run --target x86_64-unknown-linux-musl");
    
    // 检查执行结果
    if (result == -1) {
        LOG_ERROR("system call failed");
        return;
    }
    return 0;
}