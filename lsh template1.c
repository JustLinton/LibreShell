#include "lsh.h"
#include <pthread.h>
#include <fcntl.h>   
#define LENGTH 255
#define READ 0
#define WRITE 1

void redirect_file(int k,char* path,int flag); 
int redirect_pipe(int k,int pipe_id[2]);

void pipeFailure();

void catch_signal(int sign)
{
    switch(sign)
    {
    case SIGPIPE:
        printf("SIGPIPE signal\n");
        exit(0);
        break;
    }
}


int main(int argc, char *argv[]) {
//如果在命令行上没输入子进程要执行的命令 //则执行缺省的命令

      int i;
      int pid; //存放子进程号
      int status; //存放子进程返回状态
      char *args[] = {"/bin/ls","-a",NULL}; //子进程要缺省执行的命令
      char *argsT[] = {"/home/linton/Study/OS/Projs/Proj3a/mecho","",NULL}; //子进程要缺省执行的命令
	char* newPath= "/home/linton";
	char* testStr= "tStr\n";
	char *cc = (char *)malloc(10);
	int pipe1[2];

      signal(SIGINT,(sighandler_t)sigcat); //注册一个本进程处理键盘中断的函数   
      signal(SIGPIPE,(sighandler_t)catch_signal);
      // perror("SIGINT");  //如果系统调用signal成功执行，输出”SIGINT”，否则，


      //建立管道。一定要在fork之前创建管道。
	if (pipe(pipe1) < 0) pipeFailure();

      pid=fork() ; //建立子进程
      if(pid<0) // 建立子进程失败?
      {
             printf("Create Process fail!\n");
             exit(EXIT_FAILURE); 
      }

      if(pid == 0) // 子进程执行代码段 
      {
            //报告父子进程进程号
            printf("I am Child process %d\nMy father is %d\n",getpid(),getppid());            

            //测试切换目录
	      // chdir(newPath);

            //测试获取当前目录
		char pwd[LENGTH];
		char *wd;

		//getcwd函数测试		
		if(!getcwd(pwd,LENGTH)){
			perror("getcwd");
			return 1;
		}
		printf("\ngetcwd pwd is %s\n",pwd);

		// redirect_file(0,"./input.txt",O_RDONLY);
		// redirect_file(1,"./output.txt",O_RDWR);
		
		redirect_pipe(0,pipe1);
            
            status = execve(argsT[0],argsT,NULL);
       }
       else //父进程执行代码段 
       {
            printf("\nI am Parent process	%d\n",getpid()); //报告父进程进程号
                      //如果在命令行上输入了子进程要执行的命令
                      //则父进程等待子进程执行结束

            //测试父进程给子进程通过管道发送信号
		close(pipe1[READ]);  
		write(pipe1[WRITE], testStr, sizeof(testStr));
		printf("write\n");//此句不输出，说明write被阻塞？

            waitpid(pid,&status,0);	//等待子进程结束        
            printf("\nMy child exit! status = %d\n\n",status);
       }
       return EXIT_SUCCESS;
}

void redirect_file(int k,char* path,int flag){
	//重定向标准设备到文件
      int fd=open(path,flag);
      close(k); 
      dup(fd);
}

int redirect_pipe(int k,int pipe_id[2]){
	//重定向标准设备到管道 
 	if(!pipe_id) printf("invalid pipe.\n");
      close(k); 
      dup(pipe_id[k]);
//      printf("pipeid%d,%d,%d\n",pipe_id[k],pipe_id[READ],pipe_id[WRITE]);
     if(k==READ)close(pipe_id[WRITE]);
     else if (k==WRITE)close(pipe_id[READ]);
}

void pipeFailure(){
	//管道创建错误，则报错并结束
	perror("pipe  create failed\n");
	exit(EXIT_FAILURE);
}