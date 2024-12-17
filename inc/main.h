#define VERSION "1.0.0"

void run_command(const char *tag, const char *command, const char *ip, const char *mem);
void build_command(const char *tag, const char *file_name, int mode, const char *work_path, const char *aa_path);
void delete_command(const char *path);
void attestation_command();
void print_usage();