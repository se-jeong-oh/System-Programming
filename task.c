#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define MAXARGS 100
#define MAXLINE 100

extern char **environ;
int builtin_command(char *argv[]);
int parseline(char *buf, char *argv[]);
void eval(char *cmdline);
int pip_check(char *buf);
void pip_parsecmd(char *cmdline, char *piped_cmd[]);
char *trim(char *temp);
void sigint_handler(int sig);
void unix_error(char *msg);
char *Fgets(char *ptr, int n, FILE *stream);
pid_t Fork(void);
ssize_t sio_puts(char s[]);
static size_t sio_strlen(char s[]);

int main() {
    char cmdline[MAXLINE];

    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        unix_error("signal error");
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
static size_t sio_strlen(char s[]) {
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
ssize_t sio_puts(char s[]) {
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}
pid_t Fork(void) {
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
char *Fgets(char *ptr, int n, FILE *stream) {
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	    unix_error("Fgets error");

    return rptr;
}
void unix_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
void sigint_handler(int sig) {
    sio_puts("\n");
    sio_puts("CSE4100:P4-myshell>");
    return;
}
char *trim(char *temp) {
    char *org = temp;
    int start_idx = -1, end_idx = -1, i, j=0;
    for(i=0;i<strlen(org);i++) {
        if((org[i] != ' ')) {
            start_idx = i;
            break;
        }
    }
    for(i=strlen(org)-1;i>-1;i--) {
        if((org[i] != ' ')) {
            end_idx = i;
            break;
        }
    }
    for(i=start_idx;i<end_idx+1;i++) {
        temp[j] = org[i];
        j++;
    }
    temp[j] = '\0';
    return temp;
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
    int i = 0, j = 0;
    char *temp;
    char *org;
    char tar;
    while(buf[i] != '\0') {
        if(buf[i] == '\"' || buf[i] == '\'') {
            tar = buf[i];
            i++;
            while(buf[i] != tar) {
                if(buf[i] == ' ') {
                    buf[i] = tar;
                }
                i++;
            }
        }
        i++;
    }
    i = 0;
    temp = strtok(buf," ");
    while (temp != NULL) {
        if(temp[0] == '\"' || temp[0] == '\'') {
            tar = temp[0];
            temp[strlen(temp)-1] = '\0';
            temp[0] = '\0';
            temp = &temp[1];
            j = 0;
            while(temp[j] != '\0') {
                if(temp[j] == tar) {
                    temp[j] = ' ';
                }
                j++;
            }
        }
        argv[i] = temp;
        i++;
        temp = strtok(NULL," ");
    }
    argv[i] = NULL;
    return 0;
}
int pip_check(char *buf) {
    // returns number of '|'
    int i, pip_num = 0;
    for(i=0;i<strlen(buf);i++) {
        if(buf[i] == '|') 
            pip_num++;
    }
    return pip_num;
}
void pip_parsecmd(char *cmdline, char *piped_cmd[]) {
    int i = 0;
    char *temp = strtok(cmdline,"|");
    while (temp != NULL) {
        temp = trim(temp);
        piped_cmd[i] = temp;
        i++;
        temp = strtok(NULL,"|");
    }
    piped_cmd[i] = NULL;
    return;
}
void execute_cmd(char *argv[], char path[], char path2[]) { 
    strcat(path2,argv[0]);
    if (execve(path2, argv, environ) < 0) {
        strcat(path, argv[0]);
        if(execve(path, argv, environ) < 0) {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }
    return; 
}
void eval(char *cmdline) { 
    char *argv[MAXARGS]; /* Argument list execve() */ 
    char *piped_cmd[MAXLINE]; /*Store piped commands*/
    char buf[MAXLINE]; /* Holds modified command line */ 
    int bg; /* Should the job run in bg or fg? */
    char path[MAXLINE] = "/bin/";
    char path2[MAXLINE] = "/usr/bin/";
    pid_t pid; /* Process id */
    int pip_num; /* number of pipes */
    int i;
    int fd[2]; /* fd for pipe lining*/
    int curr_depth = 0; /*current depth of process tree*/
    int status; /*child status*/
    int child_pid; /*child pid*/

    strcpy(buf, cmdline);
    pip_num = pip_check(buf);
    if(pip_num == 0) {
        /* no pipes */
        bg = parseline(buf, argv); 
        if (argv[0] == NULL)
            return; /* Ignore empty lines */
        if (!builtin_command(argv)) { 
            if ((pid = Fork()) == 0) /* Child runs user job */
                execute_cmd(argv, path, path2);
            /* Parent waits for foreground job to terminate */
            if (!bg) { 
                int status; 
                if (waitpid(pid, &status, 0) < 0)
                    unix_error("waitfg: waitpid error"); 
            }
            else
                printf("%d %s", pid, cmdline); 
        }
        return; 
    }
    else {
        /* some pipes */
        pid = fork();
        if (pid < 0) unix_error("fork error.\n");
        else if(pid == 0) {
            /*command processes*/
            pip_parsecmd(cmdline, piped_cmd); /* store each command */
            /* fd[0] for read, fd[1] for write */
            while(curr_depth != pip_num) {
                if(pipe(fd)<0) unix_error("pipe error\n");
                pid = fork();
                if(pid == -1) unix_error("fork error\n");
                if (pid > 0) {
                    /*parent process*/
                    close(fd[1]);
                    dup2(fd[0],0);
                    child_pid = wait(&status);
                    bg = parseline(piped_cmd[pip_num-curr_depth], argv);
                    if (argv[0] == NULL)
                        return; /* Ignore empty lines */
                    if (!builtin_command(argv)) {
                        execute_cmd(argv, path, path2);
                        if (!bg) { 
                            int status; 
                            if (waitpid(pid, &status, 0) < 0)
                                unix_error("waitfg: waitpid error"); 
                        }
                    }
                    break;
                }
                else {
                    /*child process*/
                    curr_depth++;
                    close(fd[0]);
                    dup2(fd[1],1);
                    if(curr_depth == pip_num) {
                        /*last child*/
                        bg = parseline(piped_cmd[0], argv);
                        if (argv[0] == NULL)
                            return; /* Ignore empty lines */
                        if (!builtin_command(argv)) {
                            execute_cmd(argv, path, path2);
                            if (!bg) { 
                                int status; 
                                if (waitpid(pid, &status, 0) < 0)
                                    unix_error("waitfg: waitpid error"); 
                            }
                        }
                    }          
                }
            }
        }
        else {
            /*shell process*/
            child_pid = wait(&status);
            return;
        }
    }
}

