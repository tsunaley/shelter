#define CLIENT_DIR "bin/c_client"
#define DOCKER_INIT "default_component/docker_init"
#define BUSYBOX "tools/busybox-1.36.1/busybox"
#define CODE 1
#define ROOTFS 2

int initrd_create(const char *tag, const char *file_name, int mode, const char *work_path, const char *aa_path);

