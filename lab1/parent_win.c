#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hChildStd_ERR_Rd = NULL;
HANDLE g_hChildStd_ERR_Wr = NULL;

void closeHandles() {
    if (g_hChildStd_IN_Wr && g_hChildStd_IN_Wr != INVALID_HANDLE_VALUE) CloseHandle(g_hChildStd_IN_Wr);
    if (g_hChildStd_OUT_Rd && g_hChildStd_OUT_Rd != INVALID_HANDLE_VALUE) CloseHandle(g_hChildStd_OUT_Rd);
    if (g_hChildStd_ERR_Rd && g_hChildStd_ERR_Rd != INVALID_HANDLE_VALUE) CloseHandle(g_hChildStd_ERR_Rd);
    if (g_hChildStd_OUT_Wr && g_hChildStd_OUT_Wr != INVALID_HANDLE_VALUE) CloseHandle(g_hChildStd_OUT_Wr);
    if (g_hChildStd_ERR_Wr && g_hChildStd_ERR_Wr != INVALID_HANDLE_VALUE) CloseHandle(g_hChildStd_ERR_Wr);
}

HANDLE CreateChildProcess() {
    TCHAR szCmdline[] = TEXT("child_win.exe");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    bSuccess = CreateProcess(NULL,
        szCmdline,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo);

    if (!bSuccess) {
        fprintf(stderr, "Failed to create child process. Error: %ld\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
}

int main() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    char fileName[256];
    printf("Enter the output file name: ");
    if (fgets(fileName, sizeof(fileName), stdin) == NULL) {
        fprintf(stderr, "Failed to read file name.\n");
        return 1;
    }
    fileName[strcspn(fileName, "\n")] = 0;

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        fprintf(stderr, "Stdout pipe creation failed.\n");
        return 1;
    }
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Stdout SetHandleInformation failed.\n");
        return 1;
    }

    if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0)) {
        fprintf(stderr, "Stderr pipe creation failed.\n");
        return 1;
    }
    if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Stderr SetHandleInformation failed.\n");
        return 1;
    }

    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
        fprintf(stderr, "Stdin pipe creation failed.\n");
        return 1;
    }
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Stdin SetHandleInformation failed.\n");
        return 1;
    }

    HANDLE hChildProcess = CreateChildProcess();
    if (hChildProcess == INVALID_HANDLE_VALUE) {
        closeHandles();
        return 1;
    }

    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_ERR_Wr);

    FILE *outputFile = fopen(fileName, "w");
    if (!outputFile) {
        fprintf(stderr, "Failed to open file: %s\n", fileName);
        closeHandles();
        CloseHandle(hChildProcess);
        return 1;
    }

    DWORD dwWritten;
    char input[256];
    while (1) {
        printf("Enter numbers (e.g., 10 5) or 'exit' to quit: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "Failed to read input.\n");
            break;
        }

        if (!WriteFile(g_hChildStd_IN_Wr, input, strlen(input), &dwWritten, NULL)) {
            fprintf(stderr, "Failed to write to child stdin. Error: %ld\n", GetLastError());
            break;
        }

        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        char result[256] = {0};
        DWORD dwRead;
        char buffer[256];
        BOOL success = ReadFile(g_hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL);
        if (success && dwRead > 0) {
            buffer[dwRead] = '\0';
            strcat(result, buffer);
            while (strchr(buffer, '\n') == NULL && success && dwRead > 0) {
                success = ReadFile(g_hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL);
                if (success && dwRead > 0) {
                    buffer[dwRead] = '\0';
                    strcat(result, buffer);
                }
            }
            if (strlen(result) > 0) {
                fprintf(outputFile, "%s", result);
                fflush(outputFile);
                printf("Received from child: %s", result);
            }
        }

        DWORD bytesAvailable = 0;
        BOOL peekSuccess = PeekNamedPipe(g_hChildStd_ERR_Rd, NULL, 0, NULL, &bytesAvailable, NULL);
        if (peekSuccess && bytesAvailable > 0) {
            char errorMsg[256] = {0};
            while (bytesAvailable > 0) {
                DWORD bytesToRead = (bytesAvailable < sizeof(buffer) - 1) ? bytesAvailable : sizeof(buffer) - 1;
                BOOL readSuccess = ReadFile(g_hChildStd_ERR_Rd, buffer, bytesToRead, &dwRead, NULL);
                if (!readSuccess || dwRead == 0) {
                    break;
                }
                buffer[dwRead] = '\0';
                strcat(errorMsg, buffer);
                peekSuccess = PeekNamedPipe(g_hChildStd_ERR_Rd, NULL, 0, NULL, &bytesAvailable, NULL);
                if (!peekSuccess) {
                    break;
                }
            }
            if (strlen(errorMsg) > 0) {
                fprintf(stderr, "Child error: %s", errorMsg);
                break;
            }
        }

        DWORD exitCode;
        if (GetExitCodeProcess(hChildProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                fprintf(stderr, "Child process has terminated with exit code: %ld\n", exitCode);
                break;
            }
        } else {
            fprintf(stderr, "Failed to get child process exit code. Error: %ld\n", GetLastError());
            break;
        }
    }

    CloseHandle(g_hChildStd_IN_Wr);

    WaitForSingleObject(hChildProcess, INFINITE);

    closeHandles();
    fclose(outputFile);
    CloseHandle(hChildProcess);
    return 0;
}
