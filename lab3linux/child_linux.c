#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 256

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Could not open shared memory");
        return 1;
    }

    char *sharedMemory = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedMemory == MAP_FAILED) {
        perror("Could not map shared memory");
        return 1;
    }

    while (1) {
        if (strncmp(sharedMemory, "exit", 4) == 0) {
            break;
        }

        int a, b;
        if (sscanf(sharedMemory, "%d %d", &a, &b) == 2) {
            if (b == 0) {
                strcpy(sharedMemory, "Error: Division by zero\n");
            } else {
                double result = (double)a / b;
                sprintf(sharedMemory, "Result: %d / %d = %.6f\n", a, b, result);
            }
        } else {
            strcpy(sharedMemory, "Invalid input\n");
        }

        sharedMemory[255] = 1;

        usleep(100000); // Sleep for 100ms
    }

    munmap(sharedMemory, SHM_SIZE);
    close(shm_fd);

    return 0;
}
