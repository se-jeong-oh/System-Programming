#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int pip_check(char *buf) {
    // returns number of '|'
    int i, pip_num = 0;
    for(i=0;i<strlen(buf);i++) {
        if(buf[i] == '|') 
            pip_num++;
    }
    return pip_num;
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
void pip_parsecmd(char *cmdline, char *piped_cmd[]) {
    int i = 0;
    char *temp = strtok(cmdline,"|");
    while (temp != NULL) {
        temp = trim(temp);
        printf("trim : %s\n", temp);
        piped_cmd[i] = temp;
        i++;
        temp = strtok(NULL,"|");
    }
    piped_cmd[i] = NULL;
    return;
}
void print_arg(char *argv[]) {
    int i = 0;
    while(argv[i] != NULL) {
        printf("%d : %s\n", i, argv[i]);
        i++;
    }
    return;
}
int main() {
    char cmdline[MAXLINE];
    char *piped_cmd[MAXLINE];
    int pip_num;
    while (1) {
        /* read */
        printf("CSE4100:P4-myshell>");
        fgets(cmdline, MAXLINE, stdin);
        cmdline[strlen(cmdline)-1]='\0';
        if (feof(stdin))
            exit(0);

        pip_num = pip_check(cmdline);
        if(pip_num == 0)
            printf("%s\n", cmdline);
        else {
            pip_parsecmd(cmdline, piped_cmd);
            
        }
    }
    return 0;
}