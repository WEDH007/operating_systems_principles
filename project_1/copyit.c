#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

void display_message(int s) {
    printf("copyit: still copying...\n");
    alarm(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("copyit: Incorrect number of arguments!\n");
        printf("usage: copyit <sourcefile> <targetfile>\n");
        exit(1);
    }

    signal(SIGALRM, display_message);
    alarm(1);

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        printf("copyit: Couldn't open file %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }

    int dest = open(argv[2], O_WRONLY | O_CREAT, 0644);
    if (dest < 0) {
        printf("copyit: Couldn't open file %s: %s\n", argv[2], strerror(errno));
        close(src);
        exit(1);
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written, total_bytes = 0;

    while (1) {
        do {
            bytes_read = read(src, buffer, sizeof(buffer));
        } while (bytes_read < 0 && errno == EINTR);

        if (bytes_read < 0) {
            printf("copyit: Error reading from %s: %s\n", argv[1], strerror(errno));
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        char *ptr = buffer;
        while (bytes_read > 0) {
            do {
                bytes_written = write(dest, ptr, bytes_read);
            } while (bytes_written < 0 && errno == EINTR);

            if (bytes_written <= 0) {
                printf("copyit: Error writing to %s: %s\n", argv[2], strerror(errno));
                close(src);
                close(dest);
                exit(1);
            }

            total_bytes += bytes_written;
            bytes_read -= bytes_written;
            ptr += bytes_written;
        }
    }

    close(src);
    close(dest);
    printf("copyit: Copied %lld bytes from file %s to %s.\n", (long long)total_bytes, argv[1], argv[2]);
    return 0;
}