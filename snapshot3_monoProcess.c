#define debug

#include <sys/types.h> 
#include <wait.h> 
#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <fcntl.h>   
#include <string.h>

#define READ 0
#define WRITE 1
#define M_LENGTH 64    //CLI一行命令的最大长度
#define M_ARGS 8    //一个程序调用能包含的最大参数数量（包括程序名称本身）


typedef void (*sighandler_t) (int); 

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

int progCntr=0; //子程序的总数（用于给出prog数组长度）
struct Program
{
	char* args[M_ARGS]; //args[0]是子程序名称
	char fileStdIn,fileStdOut,fileErrOut,tubeIn,tubeOut; //4 booleans.
	char stdInFile[M_LENGTH],stdOutFile[M_LENGTH],errOutFile[M_LENGTH];
	char tmp[M_ARGS][M_LENGTH];
	char tmp2[M_ARGS][M_LENGTH];
	int argCntr,tmpCntr; //该子程序的参数总数（用于给出args,tmp数组长度）
	int pipe[2];
};
struct Program * prog;


int getline_(char s[],int lim){
	int c,i;
	i=0;
	while((c=getchar())!=EOF&&c!='\n'&&i<lim-1)
		s[i++]=c;
	s[i]='\0';
	return i;
}

void promptAndSplit(){
	char *cc = (char *)malloc(M_LENGTH);
	getline_(cc,M_LENGTH);
	char *delim = "|";
	char *p;

	//初始化，相当于构造函数
	// for(int i=0;i<M_LENGTH;i++)
	// 	for(int j=0;j<M_ARGS;j++){
	// 		prog[i].args[j]=(char *)malloc(M_LENGTH);
	// 	}

	//给每个子程序截取到其管道分隔的字符串
	int pIndex=0; //pcntr是从0开始计算，所以是实际子程序数量-1.
	p = strtok(cc, delim);
	strcpy(prog[pIndex].tmp[0],p);
	while((p= strtok(NULL, delim))){
		pIndex++;
		strcpy(prog[pIndex].tmp[0],p);
	}
	// for(int i=0;i<=pIndex;i++)printf("%s \n",prog[i].tmp[0]);
	progCntr=pIndex+1;

	//用空格对各个子程序分别继续截取
	for(int pIndex=0;pIndex<progCntr;pIndex++){
		delim=" ";
		int argIndex=0;
		p = strtok(prog[pIndex].tmp[argIndex], delim);
		strcpy(prog[pIndex].tmp[argIndex],p);
		while((p= strtok(NULL, delim))){
			argIndex++;
			strcpy(prog[pIndex].tmp[argIndex],p);
		}
		prog[pIndex].tmpCntr=argIndex+1;   //tmp数组的长度
		// for(int i=0;i<prog[pIndex].tmpCntr;i++)printf("%s \n",prog[pIndex].tmp[i]);
	}

	//对于各子程序，从其tmp数组中分析文件重定向，顺便装载其参数到其结构体中的args
	for(int pIndex=0;pIndex<progCntr;pIndex++){
			
		if(pIndex!=progCntr-1&&progCntr>1)prog[pIndex].tubeOut=1;//如果使用了管道，那么必然要开放管道
		if(pIndex!=0&&progCntr>1)prog[pIndex].tubeIn=1;

		for(int i=0;i<prog[pIndex].tmpCntr;i++){

			if(strcmp("<",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileStdIn=1;
				strcpy(prog[pIndex].stdInFile,prog[pIndex].tmp[i+1]);
				++i;
			}else if(strcmp("<<",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileStdIn=2;
				strcpy(prog[pIndex].stdInFile,prog[pIndex].tmp[i+1]);
				++i;
			}else if(strcmp(">",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileStdOut=1;
				strcpy(prog[pIndex].stdOutFile,prog[pIndex].tmp[i+1]);
				++i;
			}else if(strcmp("2>",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileErrOut=1;
				strcpy(prog[pIndex].errOutFile,prog[pIndex].tmp[i+1]);
				++i;
			}else if(strcmp(">>",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileStdOut=2;
				strcpy(prog[pIndex].stdOutFile,prog[pIndex].tmp[i+1]);
				++i;
			}else if(strcmp("&>>",prog[pIndex].tmp[i])==0){
				prog[pIndex].fileErrOut=2;
				prog[pIndex].fileStdOut=2;
				strcpy(prog[pIndex].errOutFile,prog[pIndex].tmp[i+1]);
				strcpy(prog[pIndex].stdOutFile,prog[pIndex].tmp[i+1]);
				++i;
			}else{
				//如果上面都不成立，就说明这只是一个简单的args
				strcpy(prog[pIndex].tmp2[prog[pIndex].argCntr],prog[pIndex].tmp[i]);
				//更新该子程序的args总数
				++prog[pIndex].argCntr;
			}		
		}
			//真正装载进args数组中
		for(int i=0;i<prog[pIndex].argCntr;i++){
			prog[pIndex].args[i]=(char *)malloc(M_LENGTH);
			strcpy(prog[pIndex].args[i],prog[pIndex].tmp2[i]);
		}
	}

#ifdef debug
	// debug开始：
	for(int pIndex=0;pIndex<progCntr;pIndex++){
		printf("%d%s \n",pIndex,":");
		printf("%s %d\n","tubeinBool:",prog[pIndex].tubeIn);
		printf("%s %d\n","tubeOutBool:",prog[pIndex].tubeOut);
		printf("%s %d\n","fileStdInBool:",prog[pIndex].fileStdIn);
		printf("%s %d\n","fileStdOutBool:",prog[pIndex].fileStdOut);
		printf("%s %d\n","fileErrOutBool:",prog[pIndex].fileErrOut);
		printf("%s %s\n","errOutPath:",prog[pIndex].errOutFile);
		printf("%s %s\n","stdInPath:",prog[pIndex].stdInFile);
		printf("%s %s\n","stdOutPath:",prog[pIndex].stdOutFile);
		printf("%s","args: ");
		for(int i=0;i<prog[pIndex].argCntr;i++){
			printf("%s ",prog[pIndex].args[i]);
		}
		printf("\n");
	}
#endif
}

int main(int argc, char *argv[]) {

      int i;
      int pid; //存放子进程号
      int status; //存放子进程返回状态
      char *args[] = {"/bin/ls","-a",NULL}; //子进程要缺省执行的命令
	char backGround=0;
      struct Program progCreate[M_LENGTH]={0};
      prog=progCreate;

      while (1)
      {
      	promptAndSplit();//接收用户指令，并处理完毕

		for(int pIndex=0;pIndex<progCntr;pIndex++){
			//对管道序列进行串行执行(分别建立子进程)，并允许前者和后者通过管道进行通信

			if (pipe(prog[pIndex].pipe) < 0) pipeFailure();         //建立管道。一定要在fork之前创建管道。

			pid=fork() ; //建立子进程

			signal(SIGPIPE,(sighandler_t)catch_signal); //设置捕获错误信号
			// perror("SIGINT");  //如果系统调用signal成功执行，输出”SIGINT”.

			if(pid<0) // 建立子进程失败?
			{
				printf("[ERROR] Create Process fail!\n");
				exit(EXIT_FAILURE); 
			}

			if(pid == 0) // 子进程执行代码段 
			{
				//报告父子进程进程号
				printf("I am Child process %d\nMy father is %d\n",getpid(),getppid());            
				
				//检查并重定向stdin输入管道,index-1是因为要从前一个进程的管道读入，且tubein==1则index必然>1
				if(prog[pIndex].tubeIn==1)
					redirect_pipe(0,prog[pIndex-1].pipe);

				//检查并重定向stdout输出管道
				if(prog[pIndex].tubeOut==1)
					redirect_pipe(1,prog[pIndex].pipe);

				//检查并重定向stdin文件
				if(prog[pIndex].fileStdIn==1)
					redirect_file(0,"./input.txt",O_RDONLY);

				//检查并重定向stdout文件
				if(prog[pIndex].fileStdOut==1)
					redirect_file(1,"./output.txt",O_RDWR);

				//检查并重定向stdout文件(append)
				if(prog[pIndex].fileStdOut==2)
					redirect_file(1,"./output.txt",O_RDWR|O_APPEND);

				//检查并重定向errout文件
				if(prog[pIndex].fileErrOut==1)
					redirect_file(1,"./erroutput.txt",O_RDWR);

				//检查并重定向errout文件(append)
				if(prog[pIndex].fileErrOut==2)
					redirect_file(1,"./erroutput.txt",O_RDWR|O_APPEND);				

				// status = execve(prog[pIndex].args[0],prog[pIndex].args,NULL);
				
				status = execvp(prog[pIndex].args[0],prog[pIndex].args);
				// status = execve(args[0],args,NULL);
			}
		}

		//父进程代码
            printf("\nI am Parent process	%d\n",getpid()); //报告父进程进程号

		if(backGround==0){
      		waitpid(pid,&status,0);	//等待子进程结束        
            	printf("\nMy child exit! status = %d\n\n",status);
		}

		//释放本轮程序链的args数组空间
		for(int i=0;i<M_LENGTH;i++)
			for(int j=0;j<M_ARGS;j++){
				free(prog[i].args[j]);
			}

           	prog = ( struct Program *)malloc(M_LENGTH*sizeof(struct Program));//当前指令处理完成，清理本条指令对应的子程序数组
		backGround=0;
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