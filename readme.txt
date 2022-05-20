This is the Readme file of Phase 3.

<Concept>
- When the command includes &, Run Process in background.

<Description>
- If command includes &, then Shell process doesn't wait for child process.
- When background process is finished, then Shell process catch Signal and Reap the child process.
- using some builtin commands, user can print current background jobs, 
- change current state of jobs, and kill jobs.

<Core functions>
1. void sigint_handler(int sig);
- Handler of signal SIGINT.
- Terminate the child process

2. void sigtspt_handler(int sig);
- Handler of signal SIGTSPT
- Stop the child process.

3. void sigchld_handler(int sig);
- Handler of signal SIGCHLD
- Reap the chld process.

4. void printjob(void);
- Print every background jobs with the current state.
- Current states are suspended / running.

5. void addjob(pid_t pid, char buf[]);
- Add current job to job_list.

6. void deletejob(pid_t pid);
- Delete Job from job_list.

7. void killjob(int idx);
- kill background job.

8. void bgjob(int idx);
- Change suspended background job to running background job.

9. void fgjob(int idx);
- Change suspended / running background job to running foreground job.