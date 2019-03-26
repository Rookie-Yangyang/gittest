#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc,char **argv) 
{
    pid_t       pid;
    printf("Parent process PID[%d] start running.......\n",getpid());
    


    pid=fork();
    if(pid<0)
    {
        printf("fork() create child peocess failure:%s\n",strerror(errno));
        return -1;
    }
    else if(pid==0)
    {
        printf("child process PID[%d] start running,my parent pid[%d]\n",getpid(),getppid());
        return 0;
    }
    else if(pid>0)
    {
        printf("parent process pid[%d] continue running,child process pid[%d]\n",getpid(),pid);
        return 0;
    }
} 
