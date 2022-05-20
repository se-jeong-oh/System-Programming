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
    int job_number; // index in the job list
    pid_t job_pid; // pid of the job
    int state; // state of the job(0:suspended, 1:running)
    char command[MAXARGS]; // save the command
} Job;

Job job_list[MAXJOBS];
volatile int job = -1; // numbers of the job
volatile pid_t curr_pid; // pid of foreground job
volatile int check_sigint = 0; // check for SIGINT signal in shell
volatile int check_stp = 0; // check for foreground sigstp signal (0 : no signal / 1 : signal)
volatile int check_bg = 0; // check for background job starts run (0 : no signal / 1 : signal)

extern char **environ;
int builtin_command(char *argv[]); // execute builtin commands
int parseline(char *buf, char *argv[]); // parsing command
void eval(char *cmdline); // evaluate the command
int pip_check(char *buf); // check if there are pipes in the command
void pip_parsecmd(char *cmdline, char *piped_cmd[]); // parsing command by pipe
char *trim(char *temp); // remove spaces command's front and back
void sigint_handler(int sig); // Handler of SIGINT
void unix_error(char *msg); // Print Error message
char *Fgets(char *ptr, int n, FILE *stream); // Get Command by stdin(keyboard)
pid_t Fork(void); // Fork child process
ssize_t sio_puts(char s[]); // print some messages
static size_t sio_strlen(char s[]); // calculate length of argument passed to sio_puts func
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset); // Masking Signal
void printjob(void); // Print Background Jobs with Status, Command
void addjob(pid_t pid, char buf[]); // Add Jobs in Job list
void deletejob(pid_t pid); // Delete Jobs in Job list
void killjob(int idx); // Kill Jobs in Job list
void bgjob(int idx); // Make Suspended Job to Running Background Job
void fgjob(int idx); // Make Suspended/Running Backgroun Job to Running Foreground Job
void shell_handler(int sig); // Signal Handler which runs in Shell process
void sigtspt_handler(int sig); // SIGTSTP Handler
void sigchld_handler(int sig); // SIGCHLD Handler