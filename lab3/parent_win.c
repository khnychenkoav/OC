#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "Global\\MyFileMappingObject");
    if (hMapFile == NULL) {
        fprintf(stderr, "Could not create file mapping object (%ld).\n", GetLastError());
        return 1;
    }

    char *sharedMemory = (char *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (sharedMemory == NULL) {
        fprintf(stderr, "Could not map view of file (%ld).\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    char fileName[256];
    printf("Enter the output file name: ");
    if (fgets(fileName, sizeof(fileName), stdin) == NULL) {
        fprintf(stderr, "Failed to read file name.\n");
        UnmapViewOfFile(sharedMemory);
        CloseHandle(hMapFile);
        return 1;
    }
    fileName[strcspn(fileName, "\n")] = 0;

    FILE *outputFile = fopen(fileName, "w");
    if (!outputFile) {
        fprintf(stderr, "Failed to open file: %s\n", fileName);
        UnmapViewOfFile(sharedMemory);
        CloseHandle(hMapFile);
        return 1;
    }

    STARTUPINFO siStartInfo;
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(siStartInfo);
    ZeroMemory(&piProcInfo, sizeof(piProcInfo));

    if (!CreateProcess("child_win.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
        fprintf(stderr, "Failed to create child process. Error: %ld\n", GetLastError());
        fclose(outputFile);
        UnmapViewOfFile(sharedMemory);
        CloseHandle(hMapFile);
        return 1;
    }

    char input[256];
    while (1) {
        printf("Enter numbers (e.g., 10 5) or 'exit' to quit: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "Failed to read input.\n");
            break;
        }

        strcpy(sharedMemory, input);
        sharedMemory[255] = 0;  

        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        while (sharedMemory[255] == 0) {
            Sleep(50);
        }

        fprintf(outputFile, "%s", sharedMemory);
        fflush(outputFile);
        printf("Received from child: %s", sharedMemory);
    }

    WaitForSingleObject(piProcInfo.hProcess, INFINITE);

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    fclose(outputFile);

    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);

    return 0;
}
