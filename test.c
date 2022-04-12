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
#define MAXJOBS 100

typedef struct _Job {
    int empty_flag; // 0 : current, 1 : finished (deleted)
    int job_number; // index of the job
    pid_t job_pid; // pid of the job
    int state; // state of the job(0:suspended, 1:running)
    char command[MAXARGS]; // store command
} Job;

Job job_list[MAXJOBS];
int job = -1; // numbers of the job
pid_t curr_pid; // pid of foreground job
int check_sigint = 0; // check for SIGINT signal in shell

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
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void printjob(void);
void addjob(pid_t pid, char buf[]);
void deletejob(pid_t pid);
void killjob(int idx);
void bgjob(int idx);
void fgjob(int idx);
void shell_handler(int sig);
void sigtspt_handler(int sig);
void sigchld_handler(int sig);

int main() {
    char cmdline[MAXLINE];
    struct sigaction shell_act, act_old_int, act_old_tstp;
    shell_act.sa_handler = shell_handler;
    sigemptyset(&shell_act.sa_mask);
    sigaddset(&shell_act.sa_mask, SIGCHLD);
    shell_act.sa_flags = SA_RESTART;
    sigset_t mask_empty, mask_prev, mask_sigchld;

    sigemptyset(&mask_empty);
    sigemptyset(&mask_sigchld);
    sigaddset(&mask_sigchld, SIGCHLD);
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        unix_error("signal error");
    if (signal(SIGTSTP, sigtspt_handler) == SIG_ERR)
        unix_error("signal error");
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
        unix_error("sinal error");

    while (1) {
        /* read */
        Sigprocmask(SIG_SETMASK, &mask_sigchld, &mask_prev);
        if (sigaction(SIGINT, &shell_act, &act_old_int) == -1)
            unix_error("sigint error\n");
        if (sigaction(SIGTSTP, &shell_act, &act_old_tstp) == -1)
            unix_error("sigtstp error\n");
        // printf("PID : %d\n", getpid());
        curr_pid = -1;
        sio_puts("CSE4100:P4-myshell>");
        Fgets(cmdline, MAXLINE, stdin);
        if(strcmp(cmdline,"\n")==0) continue;
        cmdline[strlen(cmdline)-1]='\0';
        Sigprocmask(SIG_SETMASK, &mask_prev, NULL);
        sigaction(SIGINT,&act_old_int, NULL);
        sigaction(SIGTSTP,&act_old_tstp, NULL);
        eval(cmdline);
    }
}
void shell_handler(int sig) {
    /*
    int olderrno = errno;
    sio_puts("\nCSE4100:P4-myshell>");
    errno = olderrno;
    */
}
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    if (sigprocmask(how, set, oldset) < 0)
	    unix_error("Sigprocmask error");
    return;
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
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask_all, mask_prev;
    sio_puts("sigint!\n");
    sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &mask_prev);
    if(curr_pid != -1)
        kill(curr_pid,SIGTERM);
    Sigprocmask(SIG_SETMASK,&mask_prev, NULL);
    //sio_puts("\n");
    errno = olderrno;
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
    char temp[100];
    int i = 0, j = 0, num;
    if(strcmp(argv[0], "exit")==0)
        exit(0);
    else if(strcmp(argv[0], "cd") == 0){
        if(chdir(argv[1]) == -1)
            unix_error("cd command Error Occured\n");
        return 1;
    }
    else if(strcmp(argv[0], "&")==0)
        return 1;
    else if(strcmp(argv[0], "jobs") == 0) {
        printjob();
        return 1;
    }
    else if(strcmp(argv[0], "kill") == 0) {
        for(i=1;argv[1][i]!='\0';i++) {
            temp[j] = argv[1][i];
        }
        num = atoi(temp);
        killjob(num);
        return 1;
    }
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
    int i = 0, j = 0, flag = 0;
    char *temp;
    char *org;
    char tar;
    buf = trim(buf);
    if(buf[strlen(buf)-1] == '&')
        flag = 1;
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
    if(*argv[i-1] == '&') {
        argv[i-1] = NULL;
        return 1;
    }
    if(flag == 1)
        return 1;
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
            if(execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
    }
    return; 
}
/*
typedef struct _Job {
    int empty_flag; // 0 : current, 1 : finished (deleted)
    int job_number; // index of the job
    pid_t job_pid; // pid of the job
    int state; // state of the job(0:suspended, 1:running)
    char command[MAXARGS]; // store command
} Job;
*/
void addjob(pid_t pid, char buf[]) {
    char *temp;
    char cmd[MAXLINE];
    char *cmdp;
    char tar;
    int i = 0, j = 0;
    job++; // increase job numbers
    job_list[job].empty_flag = 0;
    job_list[job].job_number = job;
    job_list[job].job_pid = pid;
    job_list[job].state = 1;
    cmd[0] = '\0';
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
            j = 1;
            while(temp[j+1] != '\0') {
                if(temp[j] == tar) {
                    temp[j] = ' ';
                }
                j++;
            }
        }
        //argv[i] = temp;
        strcat(cmd,temp);
        strcat(cmd," ");
        //i++;
        temp = strtok(NULL," ");
    }
    //argv[i] = NULL;
    cmdp = trim(cmd);
    strcpy(job_list[job].command, cmdp);
    return;
}
void deletejob(pid_t pid) {
    int i;
    for(i=0;i<=job;i++) {
        if(job_list[i].job_pid == pid) {
            job_list[i].empty_flag = 1;
            break;
        }
    }
    return;
}
void printjob(void) {
    int i;
    char state[100];
    for(i=0;i<=job;i++) {
        if(job_list[i].empty_flag == 0) {
            if(job_list[i].state == 0) {
                strcpy(state,"suspended");
            }
            else {
                strcpy(state,"running");
            }
            printf("[%d] %s %s\n", job_list[i].job_number, state, job_list[i].command);
        }
    }
}
void killjob(int idx) {
    pid_t pid;
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);
    pid = job_list[idx].job_pid;
    job_list[idx].empty_flag = 1;
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    deletejob(pid);
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    kill(pid, SIGKILL);
    return;
}
void bgjob(int idx) {
    return;
}
void fgjob(int idx) {
    return;
}
void sigtspt_handler(int sig) {
    int i, olderrno = errno;
    sigset_t mask_all, prev_all, mask_sigchld;
    sio_puts("sigtstp!\n");
    sigemptyset(&mask_sigchld);
    sigaddset(&mask_sigchld, SIGCHLD);
    sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    printf("JOB Number:%d\n", job);
    for(i=0;i<=job;i++) {
        printf("JOB PID : %d / curr PID : %d\n", job_list[i].job_pid, curr_pid);
        if(curr_pid == job_list[i].job_pid) {            
            job_list[i].state = 0;
            break;
        }
    }
    printf("JOB : %d\n", i);
    kill(curr_pid, SIGSTOP);
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    Sigprocmask(SIG_BLOCK, &mask_sigchld, NULL);
    errno = olderrno;
}
void sigchld_handler(int sig) {
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    int status;
    sio_puts("sigchld!\n");
    sigfillset(&mask_all);
    if((pid = waitpid(curr_pid, &status, WUNTRACED))<0) {
        //sio_puts("waitpidasa error\n");
    }
    if(!WIFSTOPPED(status)) {
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        deletejob(pid);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    errno = olderrno;
    return;
}
void eval(char *cmdline) { 
    char *argv[MAXARGS]; /* Argument list execve() */ 
    char *piped_cmd[MAXLINE]; /*Store piped commands*/
    char buf[MAXLINE]; /* Holds modified command line */ 
    int bg; /* Should the job run in bg or fg? */
    char path[MAXLINE] = "/bin/";
    char path2[MAXLINE] = "/usr/bin/";
    int pid; /* Process id */
    int pip_num; /* number of pipes */
    int i;
    int fd[2]; /* fd for pipe lining*/
    int curr_depth = 0; /*current depth of process tree*/
    int status; /*child status*/
    int child_pid; /*child pid*/
    sigset_t mask_all, mask_sigchld, mask_prev, mask_sigint, mask_tstp, mask_empty;

    sigfillset(&mask_all);
    sigemptyset(&mask_sigchld);
    sigemptyset(&mask_sigint);
    sigemptyset(&mask_tstp);
    sigemptyset(&mask_empty);
    sigaddset(&mask_sigchld, SIGCHLD);
    sigaddset(&mask_sigint, SIGINT);
    sigaddset(&mask_tstp, SIGTSTP);

    strcpy(buf, cmdline);
    pip_num = pip_check(buf);
    if(pip_num == 0) {
        /* no pipes */
        bg = parseline(buf, argv); 
        if (argv[0] == NULL)
            return; /* Ignore empty lines */
        if (!builtin_command(argv)) {
            Sigprocmask(SIG_SETMASK, &mask_sigchld, &mask_prev);
            if ((pid = Fork()) == 0) {/* Child runs user job */
                if(bg) {
                    Sigprocmask(SIG_BLOCK, &mask_sigint, NULL);
                    Sigprocmask(SIG_BLOCK, &mask_tstp, NULL);
                }
                execute_cmd(argv, path, path2);
            }
            //Sigprocmask(SIG_BLOCK, &mask_all, NULL);
            /* Parent waits for foreground job to terminate */
            addjob(pid, cmdline);
            if (!bg) {
                //Sigprocmask(SIG_SETMASK, &mask_sigchld, NULL);
                int status;
                curr_pid = pid;
                if (waitpid(pid, &status, WUNTRACED) < 0) {
                    //unix_error("waitfg: waitpid error");
                }
                else {
                    if(!WIFSTOPPED(status)) {
                        deletejob(pid);
                        job--;
                    }
                    //Sigprocmask(SIG_SETMASK, &mask_prev, NULL);
                }
            }
            else {                
                Sigprocmask(SIG_SETMASK, &mask_prev, NULL);
                //Sigprocmask(SIG_BLOCK, &mask_sigint, NULL);
                //Sigprocmask(SIG_BLOCK, &mask_sigint, NULL);
            }
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

