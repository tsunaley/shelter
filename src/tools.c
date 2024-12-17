
#include <stdlib.h>
#include "log.h"
#include "tools.h"

/*
 * 函数 create_directory() 用于创建目录
 * 参数:
 *   - const char *path: 目录的路径
 * 返回值:
 *   - int: 创建目录的结果，始终返回0
 */
int create_directory(const char *path) {
    if (mkdir(path, 0777) != 0) {
        LOG_ERROR("Failed to create directory: %s (%s)", path);
    }
    return 0;
}

/*
 * 函数 write_to_file() 将内容写入指定的文件
 * 参数:
 *   - const char *path: 文件的路径
 *   - const char *content: 要写入的内容
 * 返回值:
 *   - int: 写入文件的结果，始终返回0
 */
int write_to_file(const char *path, const char *content) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        LOG_ERROR("Failed to open file: %s\n", path);
    }
    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}

/*
 * 函数 get_file_name() 从路径中提取文件名
 * 参数:
 *   - char* path: 文件路径
 * 返回值:
 *   - char*: 提取出的文件名
 */
char* get_file_name(char* path){
    char *lastSlash;
    char *result;
    lastSlash = strrchr(path, '/');
    
    if (lastSlash != NULL) {
        result = lastSlash + 1;
    } else {
        result = path;
    }
    return result;
}