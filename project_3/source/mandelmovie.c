#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number of processes>\n", argv[0]);
        return 1;
    }

    int n_processes = atoi(argv[1]);
    if (n_processes <= 0)
    {
        fprintf(stderr, "Error: Invalid number of processes.\n");
        return 1;
    }

    double scale_step = (2.0 - 0.0001) / 49.0; // assuming final scale is 0.05 and initial scale is 2.0
    double current_scale = 2.0;

    for (int i = 1; i <= 50;)
    {
        // Start n_processes at a time
        for (int j = 0; j < n_processes && i <= 50; ++j, ++i)
        {
            pid_t pid = fork();
            if (pid == 0)
            { // child process
                char filename[50];
                snprintf(filename, sizeof(filename), "mandel%d.bmp", i);
                char scale[20];
                snprintf(scale, sizeof(scale), "%f", current_scale);

                execl("./mandel", "mandel", "-s", scale, "-x", "-0.5397949000000", "-y", "-0.6095890009734", "-m", "3500", "-o", filename, (char *)NULL);
                perror("execl");
                exit(1);

                // Mandelmovie executing mandel with -n 10
                // execl("./mandel", "mandel", "-s", scale, "-x", "-0.5397949000000", "-y", "-0.6095890009734", "-m", "3500", "-n", "10", "-o", filename, (char *)NULL);
                // perror("execl");
                // exit(1);
            }
            else if (pid < 0)
            {
                perror("fork");
                exit(1);
            }

            current_scale -= scale_step;
        }

        // Wait for processes to complete
        for (int j = 0; j < n_processes && i <= 50; ++j)
        {
            wait(NULL);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("number of processes: %d , time taken: %f\n", n_processes, elapsed);

    return 0;
}
