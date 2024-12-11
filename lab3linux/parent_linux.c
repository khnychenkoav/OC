#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 256

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Could not create shared memory");
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("Could not set shared memory size");
        return 1;
    }

    char *sharedMemory = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedMemory == MAP_FAILED) {
        perror("Could not map shared memory");
        return 1;
    }

    char fileName[256];
    printf("Enter the output file name: ");
    if (fgets(fileName, sizeof(fileName), stdin) == NULL) {
        perror("Failed to read file name");
        return 1;
    }
    fileName[strcspn(fileName, "\n")] = 0;

    FILE *outputFile = fopen(fileName, "w");
    if (!outputFile) {
        perror("Failed to open file");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        return 1;
    } else if (pid == 0) {
        execl("./child_linux", "./child_linux", NULL);
        perror("Failed to execute child process");
        return 1;
    }

    char input[256];
    while (1) {
        printf("Enter numbers (e.g., 10 5) or 'exit' to quit: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Failed to read input");
            break;
        }

        strcpy(sharedMemory, input);
        sharedMemory[255] = 0;

        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        while (sharedMemory[255] == 0) {
            usleep(50000); // Sleep for 50ms
        }

        fprintf(outputFile, "%s", sharedMemory);
        fflush(outputFile);
        printf("Received from child: %s", sharedMemory);
    }

    wait(NULL);

    fclose(outputFile);
    munmap(sharedMemory, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}
