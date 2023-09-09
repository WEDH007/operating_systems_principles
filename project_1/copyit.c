#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

// Function to display a periodic "still copying" message.
void display_message(int s) {
    printf("copyit: still copying...\n");
    alarm(1);  // Reset the alarm for another second
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("copyit: Incorrect number of arguments!\n");
        printf("usage: copyit <sourcefile> <targetfile>\n");
        exit(1);
    }

    // Set up the handler for the SIGALRM signal
    signal(SIGALRM, display_message);
    alarm(1);  // Set the initial alarm for one second

    int src = open(argv[1], O_RDONLY);
    if (src < 0) {
        printf("copyit: Couldn't open source file %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }

    int dest = open(argv[2], O_WRONLY | O_CREAT, 0644);
    if (dest < 0) {
        printf("copyit: Couldn't open or create target file %s: %s\n", argv[2], strerror(errno));
        close(src);  // Ensure the source file is closed if we fail to open the destination file
        exit(1);
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written, total_bytes = 0;

    // Begin the file copy process
    while (1) {
        do {
            bytes_read = read(src, buffer, sizeof(buffer));
        } while (bytes_read < 0 && errno == EINTR);  // Retry if read is interrupted

        if (bytes_read < 0) {
            printf("copyit: Error reading from %s: %s\n", argv[1], strerror(errno));
            break;
        }

        if (bytes_read == 0) {
            break;  // End of file reached
        }

        char *ptr = buffer;
        while (bytes_read > 0) {
            do {
                bytes_written = write(dest, ptr, bytes_read);
            } while (bytes_written < 0 && errno == EINTR);  // Retry if write is interrupted

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

    // Clean up: Close open files and report success
    close(src);
    close(dest);
    printf("copyit: Copied %lld bytes from file %s to %s.\n", (long long)total_bytes, argv[1], argv[2]);
    return 0;
}
