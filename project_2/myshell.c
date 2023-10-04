#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_SIZE 4096
#define MAX_WORDS 100

char input[MAX_INPUT_SIZE];
char *words[MAX_WORDS];

void read_input() {
    printf("myshell> ");
    fflush(stdout);
    if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
        exit(0); // Exit on EOF
    }
    char *token = strtok(input, " \t\n");
    int i = 0;
    while (token) {
        words[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    words[i] = NULL;
}

void execute_command() {
    pid_t pid, wpid;
    int status;

    if (strcmp(words[0], "start") == 0) {
        if ((pid = fork()) == 0) {
            execvp(words[1], &words[1]);
            perror("myshell");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            printf("myshell: process %d started\n", pid);
        } else {
            perror("myshell");
        }
    } else if (strcmp(words[0], "wait") == 0) {
        wpid = wait(&status);
        if (WIFEXITED(status)) {
            printf("myshell: process %d exited normally with status %d\n", wpid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("myshell: process %d exited abnormally with signal %d: %s\n", wpid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        } else {
            printf("myshell: no processes left\n");
        }
    } else if (strcmp(words[0], "run") == 0) {
        if ((pid = fork()) == 0) {
            execvp(words[1], &words[1]);
            perror("myshell");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            wpid = waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("myshell: process %d exited normally with status %d\n", wpid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("myshell: process %d exited abnormally with signal %d: %s\n", wpid, WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
        } else {
            perror("myshell");
        }
    } else if (strcmp(words[0], "kill") == 0) {
        if (kill(atoi(words[1]), SIGKILL) == 0) {
            printf("myshell: process %s killed\n", words[1]);
        } else {
            perror("myshell");
        }
    } else if (strcmp(words[0], "stop") == 0) {
        if (kill(atoi(words[1]), SIGSTOP) == 0) {
            printf("myshell: process %s stopped\n", words[1]);
        } else {
            perror("myshell");
        }
    } else if (strcmp(words[0], "continue") == 0) {
        if (kill(atoi(words[1]), SIGCONT) == 0) {
            printf("myshell: process %s continued\n", words[1]);
        } else {
            perror("myshell");
        }
    } else if (strcmp(words[0], "exit") == 0 || strcmp(words[0], "quit") == 0) {
        exit(0);
    } else {
        printf("myshell: unknown command: %s\n", words[0]);
    }
}

int main() {
    while (1) {
        read_input();
        execute_command();
    }
    return 0;
}
