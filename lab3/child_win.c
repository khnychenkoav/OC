#include <stdio.h>
#include <windows.h>
#include <string.h>

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "Global\\MyFileMappingObject");
    if (hMapFile == NULL) {
        fprintf(stderr, "Could not open file mapping object (%ld).\n", GetLastError());
        return 1;
    }

    char *sharedMemory = (char *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (sharedMemory == NULL) {
        fprintf(stderr, "Could not map view of file (%ld).\n", GetLastError());
        CloseHandle(hMapFile);
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

        Sleep(100); 
    }

    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);

    return 0;
}
