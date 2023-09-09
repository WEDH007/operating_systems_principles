#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

void display_message(int s) {
    printf("copyit: still copying...\n");
    alarm(1);
}

int copy_file(const char *src_path, const char *dest_path);
int copy_recursive(const char *src_path, const char *dest_path);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("copyit: Incorrect number of arguments!\n");
        printf("usage: copyit_extracredit <source> <target>\n");
        exit(1);
    }

    signal(SIGALRM, display_message);
    alarm(1);

    if (copy_recursive(argv[1], argv[2]) != 0) {
        printf("copyit: Error during copying.\n");
        exit(1);
    }

    printf("copyit: Copying completed.\n");
    return 0;
}

int copy_recursive(const char *src_path, const char *dest_path) {
    struct stat st;
    if (stat(src_path, &st) != 0) {
        perror("copyit: stat failed");
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(src_path);
        if (!dir) {
            perror("copyit: opendir failed");
            return -1;
        }

        if (mkdir(dest_path, st.st_mode) != 0 && errno != EEXIST) {
            perror("copyit: mkdir failed");
            closedir(dir);
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char new_src_path[PATH_MAX], new_dest_path[PATH_MAX];
            snprintf(new_src_path, sizeof(new_src_path), "%s/%s", src_path, entry->d_name);
            snprintf(new_dest_path, sizeof(new_dest_path), "%s/%s", dest_path, entry->d_name);

            if (copy_recursive(new_src_path, new_dest_path) != 0) {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        return copy_file(src_path, dest_path);
    } else {
        printf("copyit: Skipping non-regular file: %s\n", src_path);
    }

    return 0;
}

int copy_file(const char *src_path, const char *dest_path) {
    int src = open(src_path, O_RDONLY);
    if (src < 0) {
        perror("copyit: Error opening source file");
        return -1;
    }

    int dest = open(dest_path, O_WRONLY | O_CREAT, 0644);
    if (dest < 0) {
        perror("copyit: Error opening destination file");
        close(src);
        return -1;
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written;

    while (1) {
        do {
            bytes_read = read(src, buffer, sizeof(buffer));
        } while (bytes_read < 0 && errno == EINTR);

        if (bytes_read < 0) {
            perror("copyit: Error reading source file");
            close(src);
            close(dest);
            return -1;
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
                perror("copyit: Error writing to destination file");
                close(src);
                close(dest);
                return -1;
            }

            bytes_read -= bytes_written;
            ptr += bytes_written;
        }
    }

    close(src);
    close(dest);
    return 0;
}
