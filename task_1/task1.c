#include "csapp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define MAXARGS 100
extern char **environ;
int builtin_command(char *argv[]);
int parseline(char *buf, char *argv[]);
void eval(char *cmdline);
int main() {
    char cmdline[MAXLINE];

    while (1) {
        /* read */
        printf("CSE4100:P4-myshell>");
        Fgets(cmdline, MAXLINE, stdin);
        cmdline[strlen(cmdline)-1]='\0';
        if (feof(stdin))
            exit(0);

        eval(cmdline);
    }
}
int builtin_command(char *argv[]) {
    if(strcmp(argv[0], "exit")==0)
        exit(0);
    else if(strcmp(argv[0], "cd") == 0){
        if(chdir(argv[1]) == -1)
            unix_error("cd command Error Occured\n");
        return 1;
    }
    else if(strcmp(argv[0], "&")==0)
        return 1;
    return 0;
}
void print_arg(char *argv[]) {
    int i = 0;
    while(argv[i] != NULL) {
        printf("%d : %s\n", i, argv[i]);
        i++;
    }
    return;
}
int parseline(char *buf, char *argv[]) {
    int i = 0;
    char *temp = strtok(buf," ");
    while (temp != NULL) {
        argv[i] = temp;
        i++;
        temp = strtok(NULL," ");
    }
    argv[i] = NULL;
    //print_arg(argv);
    return 0;
}
void eval(char *cmdline)
{ 
    char *argv[MAXARGS]; /* Argument list execve() */ 
    char buf[MAXLINE]; /* Holds modified command line */ 
    int bg; /* Should the job run in bg or fg? */
    char path[MAXLINE] = "/bin/";
    pid_t pid; /* Process id */
    
    strcpy(buf, cmdline); 
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)
        return; /* Ignore empty lines */
    if (!builtin_command(argv)) { 
        if ((pid = Fork()) == 0) { /* Child runs user job */
            strcat(path,argv[0]);
            if (execve(path, argv, environ) < 0) { 
                printf("%s: Command not found.\n", argv[0]);
                exit(0); 
            } 
        }
    /* Parent waits for foreground job to terminate */
    if (!bg) { 
        int status; 
        if (waitpid(pid, &status, 0) < 0)
            unix_error("waitfg: waitpid error"); 
    }
    else
        printf("%d %s", pid, cmdline); }
    return; 
}

