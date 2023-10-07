#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 4096
#define MAX_WORDS 100

char input[MAX_INPUT_SIZE];
char *words[MAX_WORDS];

void read_input() {
    printf("myshell> ");
    fflush(stdout);

    if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
        if (feof(stdin)) {
            printf("myshell: End of input detected. Exiting.\n");
            exit(0);
        } else {
            perror("myshell: Error reading input");
            exit(EXIT_FAILURE);
        }
    }

    char *token = strtok(input, " \t\n");
    int i = 0;
    while (token && i < MAX_WORDS - 1) {
        words[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    words[i] = NULL;
}

void execute_command() {
    pid_t pid, wpid;
    int status;
    int in_fd, out_fd;

    if (!words[0]) {
        return;
    } else if (strcmp(words[0], "start") == 0) {
        char *program = NULL;
        int j;
        for (j = 1; j < MAX_WORDS && words[j]; j++) {
            if (words[j]) {
                program = words[j];
                break;
            }
        }
        if (!program) {
            printf("myshell: 'start' requires a program to execute.\n");
            return;
        }

        if ((pid = fork()) == 0) {
            for (int i = 1; words[i]; i++) {
                if (strcmp(words[i], "<") == 0 && words[i + 1]) {
                    printf("Attempting to open input file: %s\n", words[i + 1]);
                    in_fd = open(words[i + 1], O_RDONLY);
                    if (in_fd == -1) {
                        perror("myshell: Error opening input file");
                        exit(EXIT_FAILURE);
                    }
                    printf("Input file opened with fd: %d\n", in_fd);
                    if (dup2(in_fd, STDIN_FILENO) == -1) {
                        perror("myshell: Error redirecting input");
                        exit(EXIT_FAILURE);
                    }
                    close(in_fd);
                    words[i] = NULL;
                    words[i+1] = NULL;
                }

                if (strcmp(words[i], ">") == 0 && words[i + 1]) {
                    printf("Attempting to open output file: %s\n", words[i + 1]);
                    out_fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (out_fd == -1) {
                        perror("myshell: Error opening output file");
                        exit(EXIT_FAILURE);
                    }
                    printf("Output file opened with fd: %d\n", out_fd);
                    if (dup2(out_fd, STDOUT_FILENO) == -1) {
                        perror("myshell: Error redirecting output");
                        exit(EXIT_FAILURE);
                    }
                    close(out_fd);
                    words[i] = NULL;
                    words[i+1] = NULL;
                }
            }

            execvp(program, &words[j]);
            perror("myshell: Error executing command");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("myshell: Error starting process");
        } else {
            printf("myshell: process %d started\n", pid);
        }
    } else if (strcmp(words[0], "wait") == 0) {
        wpid = wait(&status);
        if (wpid < 0) {
            perror("myshell: Error waiting for process");
            return;
        }
        if (WIFEXITED(status)) {
            printf("myshell: process %d exited normally with status %d\n", wpid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("myshell: process %d exited abnormally with signal %d: %s\n", wpid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        }
    } else if (strcmp(words[0], "run") == 0) {
        if (!words[1]) {
            printf("myshell: 'run' requires a program to execute.\n");
            return;
        }

        if ((pid = fork()) == 0) {
            execvp(words[1], &words[1]);
            perror("myshell: Error executing command");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("myshell: Error starting process");
        } else {
            wpid = waitpid(pid, &status, 0);
            if (wpid < 0) {
                perror("myshell: Error waiting for process");
                return;
            }
            if (WIFEXITED(status)) {
                printf("myshell: process %d exited normally with status %d\n", wpid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("myshell: process %d exited abnormally with signal %d: %s\n", wpid, WTERMSIG(status), strsignal(WTERMSIG(status)));
            }
        }
    } else if (strcmp(words[0], "kill") == 0 || strcmp(words[0], "stop") == 0 || strcmp(words[0], "continue") == 0) {
        if (!words[1]) {
            printf("myshell: Command '%s' requires a process ID.\n", words[0]);
            return;
        }
        int sig;
        if (strcmp(words[0], "kill") == 0) {
            sig = SIGKILL;
        } else if (strcmp(words[0], "stop") == 0) {
            sig = SIGSTOP;
        } else {
            sig = SIGCONT;
        }

        pid_t target_pid = atoi(words[1]);
        if (kill(target_pid, sig) < 0) {
            perror("myshell: Error sending signal to process");
        } else {
            printf("myshell: signal %d sent to process %d\n", sig, target_pid);
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
