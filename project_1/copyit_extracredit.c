#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

void display_message(int s) {
    printf("copyit_extracredit: still copying...\n");
    alarm(1);
}

ssize_t copy_file(const char *src_path, const char *dest_path) {
    int src = open(src_path, O_RDONLY);
    if (src < 0) {
        perror("copyit_extracredit: Error opening source file");
        return -1;
    }

    int dest = open(dest_path, O_WRONLY | O_CREAT, 0644);
    if (dest < 0) {
        perror("copyit_extracredit: Error opening destination file");
        close(src);
        return -1;
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written, total_bytes = 0;

    while ((bytes_read = read(src, buffer, sizeof(buffer))) > 0) {
        char *ptr = buffer;
        while (bytes_read > 0) {
            bytes_written = write(dest, ptr, bytes_read);
            if (bytes_written <= 0) {
                perror("copyit_extracredit: Write error");
                close(src);
                close(dest);
                return -1;
            }
            total_bytes += bytes_written;
            bytes_read -= bytes_written;
            ptr += bytes_written;
        }
    }

    if (bytes_read < 0) {
        perror("copyit_extracredit: Read error");
    }

    close(src);
    close(dest);
    return total_bytes;
}

void copy_directory_recursive(const char *src_dir, const char *dest_dir) {
    struct stat st;
    DIR *dir = opendir(src_dir);
    struct dirent *entry;

    if (!dir) {
        perror("copyit_extracredit: Could not open source directory");
        return;
    }

    if (mkdir(dest_dir, 0755) != 0 && errno != EEXIST) {
        perror("copyit_extracredit: Could not create destination directory");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];

        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                copy_directory_recursive(src_path, dest_path);
            } else if (S_ISREG(st.st_mode)) {
                copy_file(src_path, dest_path);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("copyit_extracredit: Incorrect number of arguments!\n");
        printf("usage: copyit_extracredit <sourcepath> <targetpath>\n");
        exit(1);
    }

    signal(SIGALRM, display_message);
    alarm(1);

    struct stat st;
    if (stat(argv[1], &st) != 0) {
        perror("copyit_extracredit: Error stat'ing source");
        exit(1);
    }

    if (S_ISDIR(st.st_mode)) {
        copy_directory_recursive(argv[1], argv[2]);
    } else if (S_ISREG(st.st_mode)) {
        copy_file(argv[1], argv[2]);
    } else {
        fprintf(stderr, "copyit_extracredit: Source is neither a regular file nor a directory!\n");
        exit(1);
    }

    return 0;
}
