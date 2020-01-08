#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
int main()
{
    pid_t pid;
    int status = 111;
    if((pid = fork()) == 0)
    {
        printf("Hi I am child process!\n");
        sleep(10);
        return 0;
    }
    else
    {
        printf("send signal to child process (%d) \n", pid);
        sleep(5);
        kill(pid, SIGINT);
        wait(&status);
        if(WIFSIGNALED(status))
            printf("chile process receive signal %d\n", WTERMSIG(status));
    }
}