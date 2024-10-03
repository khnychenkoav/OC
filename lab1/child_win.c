#include <stdio.h>
#include <windows.h>
#include <string.h>

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);

    DWORD dwRead, bytesWritten;
    while (1) {
        char input[256];
        if (!ReadFile(hStdIn, input, sizeof(input) - 1, &dwRead, NULL)) {
            fprintf(stderr, "Failed to read input. Error: %ld\n", GetLastError());
            break;
        }
        if (dwRead == 0) {
            break;
        }
        input[dwRead] = '\0';
        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        int a, b;
        if (sscanf(input, "%d %d", &a, &b) == 2) {
            if (b == 0) {
                char errMsg[] = "Division by zero. Terminating.\n";
                WriteFile(hStdErr, errMsg, strlen(errMsg), &bytesWritten, NULL);
                break;
            }
            double result = (double)a / b;
            char resultMessage[256];
            sprintf(resultMessage, "Result: %d / %d = %.6f\n", a, b, result);
            WriteFile(hStdOut, resultMessage, strlen(resultMessage), &bytesWritten, NULL);
        } else {
            char invalid[] = "Invalid input\n";
            WriteFile(hStdErr, invalid, strlen(invalid), &bytesWritten, NULL);
            break;
        }
    }

    CloseHandle(hStdIn);
    CloseHandle(hStdOut);
    CloseHandle(hStdErr);

    return 0;
}
