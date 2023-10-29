#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int num_processes = atoi(argv[1]);
    int active_processes = 0;
    for (int i = 0; i < 50; i++) {
        if (active_processes >= num_processes) {
            wait(NULL); // wait for a process to finish
            active_processes--;
        }
        pid_t pid = fork();
        if (pid == 0) { // Child process
            char filename[20];
            sprintf(filename, "mandel%d.bmp", i + 1);
            // Modify the scale factor or other parameters accordingly
            execlp("./mandel", "mandel", "-o", filename, NULL);
            exit(0); // Exit child process after execution
        } else if (pid > 0) { // Parent process
            active_processes++;
        } else {
            perror("fork failed");
            exit(1);
        }
    }

    // Wait for all child processes to finish
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }

    return 0;
}
