#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contract.h"

int main() {
    char input[256];

    while (1) {
        printf("Enter command: ");
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0';

        char* cmd = strtok(input, " ");
        if (cmd == NULL)
            continue;

        if (strcmp(cmd, "1") == 0) {
            char* arg1 = strtok(NULL, " ");
            char* arg2 = strtok(NULL, " ");

            if (arg1 == NULL || arg2 == NULL) {
                printf("Error: Expected two arguments for command 1\n");
                continue;
            }

            double A = atof(arg1);
            double deltaX = atof(arg2);
            double result = Derivative(A, deltaX);
            printf("Derivative at A=%.5f with deltaX=%.5f is %.10f\n", A, deltaX, result);
        } else if (strcmp(cmd, "2") == 0) {
            // Command 2: Pi
            char* arg1 = strtok(NULL, " ");

            if (arg1 == NULL) {
                printf("Error: Expected one argument for command 2\n");
                continue;
            }

            int K = atoi(arg1);
            double result = Pi(K);
            printf("Pi calculated with K=%d is %.15f\n", K, result);
        } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}
