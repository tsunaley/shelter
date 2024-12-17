#include "initrd_creator.h"
#include "log.h"
#include "tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include <string.h>

/*
 * 函数 initrd_create() 用于创建initrd
 * 参数:
 *   - const char *tag: 敏感程序标签
 *   - const char *file_name: 传入的文件名
 *   - int mode: 指定传入的是敏感程序（CODE）还是根文件系统（ROOTFS）
 * 返回值:
 *   - int: 正常结束返回0
 */
int initrd_create(const char *tag, const char *file_name, int mode, const char *work_path, const char *aa_path){
    char path[1024];
    char bin_path[1024];
    char *client = CLIENT_DIR;
    snprintf(path, sizeof(path), "build/%s", tag);
    create_directory(path);

    snprintf(path, sizeof(path), "build/%s/initrd", tag);
    create_directory(path);
    
    snprintf(bin_path, sizeof(bin_path), "%s/bin", path);
    create_directory(bin_path);

    char commands[4096];

    // 构建initrd需要执行的命令
    snprintf(commands, sizeof(commands), 
        "cp %s %s && "
        "cp %s %s/ && "
        "ln -s busybox %s/sh && "
        "cp %s %s/ && "
        "cp %s %s/",
        BUSYBOX, bin_path,
        DOCKER_INIT, bin_path,
        bin_path,
        client, bin_path,
        file_name, bin_path
    );  
    if (system(commands) == -1) {
        LOG_ERROR("Failed to exec commands: %s", commands);
    }
    if(aa_path!=NULL){
        snprintf(commands, sizeof(commands), "cp %s %s/", aa_path, bin_path);
        if (system(commands) == -1) {
        LOG_ERROR("Failed to exec commands: %s", commands);
    }
    }
    
    char init[512];
    snprintf(init, sizeof(init), "%s/init", path);
    FILE *file = fopen(init, "w");
    if (file == NULL) {
        LOG_ERROR("Failed to create init file: %s", init);
    }
    char init_script[4096] = "#!/bin/sh\n"
                "busybox ip addr add 192.168.50.10/24 dev eth0\n"
                "busybox ip link set eth0 up\n"
                "busybox ip route add default via 192.168.50.1\n"
                "busybox mknod /dev/null c 1 3\n"
                "busybox chmod 666 /dev/null\n"
                "./bin/attestation-agent &\n";


    // 根据输入文件的不同来构建不同的初始进程
    switch (mode)
    {
    case CODE:
        strcat(init_script, 
                "/bin/c_client\n"
                "exec /bin/sh \n"
                // "busybox mkdir -p /proc /sys /mnt/rootfs\n"
                // "busybox mount -t proc none /proc\n"
                // "busybox mount -t sysfs none /sys\n"
                // "echo o > /proc/sysrq-trigger\n"
            );
        break;
    case ROOTFS:
        char *name = get_file_name(file_name);
        snprintf(commands, sizeof(commands), "split -b 300M %s/%s %s/rootfs_part_ && rm %s/%s && sed -i '2i cd %s' %s/docker_init", bin_path, name, bin_path, bin_path, name, work_path, bin_path);
        // printf("%s\n", commands);
        if(system(commands)==-1){
            LOG_ERROR("Failed to exec commands: %s", commands);
        }
        snprintf(init_script + strlen(init_script), sizeof(init_script) - strlen(init_script),
                "busybox mkdir -p /proc /sys /mnt/rootfs\n"
                "busybox mount -t proc none /proc\n"
                "busybox mount -t sysfs none /sys\n"
                "busybox cat /bin/rootfs_part_* > /bin/rootfs.tar\n"
                "busybox rm /bin/rootfs_part_*\n"
                "busybox tar -xpf /bin/rootfs.tar -C /mnt/rootfs\n"
                "busybox mount -t tmpfs none /mnt/rootfs/dev\n"
                "busybox mdev -s\n"
                "busybox mv /bin/c_client /mnt/rootfs/bin/\n"
                "busybox chmod +x /mnt/rootfs/bin/c_client\n"
                "busybox mv /bin/docker_init /mnt/rootfs/bin/\n"
                "busybox chmod +x /mnt/rootfs/bin/docker_init\n"
                "busybox chroot /mnt/rootfs /bin/docker_init\n"
            );
        break;
    default:
        LOG_ERROR("Unknown mode: %d", mode);
    }

    fprintf(file, "%s", init_script);
    fclose(file);

    // 将initrd打包
    snprintf(commands, sizeof(commands), 
        "sudo chmod +x %s && "
        "cd build/%s/initrd;find . | cpio -o -H newc | gzip > ../initrd.img"
        , init, tag);
    if(system(commands)==-1){
        LOG_ERROR("Failed to exec commands: %s", commands);
    }

    return 0;
}