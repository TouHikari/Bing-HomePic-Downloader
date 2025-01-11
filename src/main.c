#include "../include/api.h"

extern FILE *log_file;

int main(void)
{
#ifdef _WIN32
#include <windows.h>
	system("chcp 65001");
#endif

	// 打开日志文件
    log_file = fopen("program.log", "a");
    if (!log_file)
	{
        fprintf(stderr, "Could not open log file for writing\n");
        return 1;
    }

    int days;
    printf("请输入要获取的天数（0 = 今天, 1 = 昨天, 2 = 前天，依此类推）：");
    scanf("%d", &days);

    api(days);

	// 关闭日志文件
    close_log();

    return 0;
}