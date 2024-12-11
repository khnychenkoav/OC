#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int main() {
    int pipe_in[2], pipe_out[2], pipe_err[2];

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1 || pipe(pipe_err) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Дочерний процесс
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_err[0]);

        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_err[1], STDERR_FILENO);

        close(pipe_in[0]);
        close(pipe_out[1]);
        close(pipe_err[1]);

        execl("./child_linux", "./child_linux", NULL);
        perror("execl");
        exit(1);
    } else {
        // Родительский процесс
        close(pipe_in[0]);
        close(pipe_out[1]);
        close(pipe_err[1]);

        if (set_nonblocking(pipe_out[0]) == -1 || set_nonblocking(pipe_err[0]) == -1) {
            fprintf(stderr, "Failed to set non-blocking mode.\n");
            return 1;
        }

        FILE *output_file;
        char file_name[256];
        printf("Enter the output file name: ");
        if (fgets(file_name, sizeof(file_name), stdin) == NULL) {
            fprintf(stderr, "Failed to read file name.\n");
            return 1;
        }
        file_name[strcspn(file_name, "\n")] = 0;

        output_file = fopen(file_name, "w");
        if (!output_file) {
            perror("fopen");
            return 1;
        }

        char input[256];
        while (1) {
            printf("Enter numbers (e.g., 10 5) or 'exit' to quit: ");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                fprintf(stderr, "Failed to read input.\n");
                break;
            }

            write(pipe_in[1], input, strlen(input));
            if (strncmp(input, "exit", 4) == 0) {
                break;
            }

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(pipe_out[0], &readfds);
            FD_SET(pipe_err[0], &readfds);

            int max_fd = (pipe_out[0] > pipe_err[0] ? pipe_out[0] : pipe_err[0]) + 1;

            struct timeval timeout;
            timeout.tv_sec = 1;  // Таймаут 1 секунда
            timeout.tv_usec = 0;

            int ready = select(max_fd, &readfds, NULL, NULL, &timeout);
            if (ready == -1) {
                perror("select");
                break;
            } else if (ready == 0) {
                fprintf(stderr, "Timeout waiting for child process.\n");
                continue;
            }

            char buffer[256];
            ssize_t bytes_read;

            if (FD_ISSET(pipe_out[0], &readfds)) {
                bytes_read = read(pipe_out[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    fprintf(output_file, "%s", buffer);
                    fflush(output_file);
                    printf("Received from child: %s", buffer);
                }
            }

            if (FD_ISSET(pipe_err[0], &readfds)) {
                bytes_read = read(pipe_err[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    fprintf(stderr, "Child error: %s", buffer);
                }
            }
        }

        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_err[0]);
        fclose(output_file);

        wait(NULL);
    }

    return 0;
}
