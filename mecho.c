#include "lsh.h"

char* str;

int main(int argc, char *argv[]) {
//如果在命令行上没输入子进程要执行的命令
	str = (char *)malloc(255);
	scanf("%s",str);
	printf("out: %s\n ",str);
	exit(EXIT_SUCCESS);
}