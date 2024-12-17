#define INITRD_DIR "default_component/initrd.img"
#define IMAGE_DIR "default_component/bzImage"
#define OVMF_DIR "default_component/OVMF.fd"
#define VSOCK_SERVER_DIR "bin/b_relay"
#define QEMU_START_DIR "default_component/start.sh"
#define SEVCTL_DIR "tools/sevctl/target/x86_64-unknown-linux-musl/release/sevctl"
#define CERTC_HAIN "bin/check_cert_chain"
#define RA "bin/remote_attestation"

int make_dockerfile();
int image_built(const char *tag);
int run_container(const char *tag);
int delet_container();
int delet_image();
int qemu_command(); // 生成对应的qemu指令
char *check_shelter_container();
void shelter_init(const char *tag);
void start_container(const char *container_id);
