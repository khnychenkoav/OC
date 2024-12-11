#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef double (*DerivativeFunc)(double, double);
typedef double (*PiFunc)(int);

int main() {
    void* handle = NULL;
    DerivativeFunc Derivative = NULL;
    PiFunc Pi = NULL;
    int current_lib = 0;

    const char* lib_paths[] = {"./libimpl1.so", "./libimpl2.so"};

    handle = dlopen(lib_paths[current_lib], RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    Derivative = (DerivativeFunc)dlsym(handle, "Derivative");
    Pi = (PiFunc)dlsym(handle, "Pi");

    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error locating symbols: %s\n", error);
        exit(EXIT_FAILURE);
    }

    char input[256];

    while (1) {
        printf("Enter command: ");
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0';

        char* cmd = strtok(input, " ");
        if (cmd == NULL)
            continue;

        if (strcmp(cmd, "0") == 0) {
            dlclose(handle);
            current_lib = 1 - current_lib;

            handle = dlopen(lib_paths[current_lib], RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "Error loading library: %s\n", dlerror());
                exit(EXIT_FAILURE);
            }

            Derivative = (DerivativeFunc)dlsym(handle, "Derivative");
            Pi = (PiFunc)dlsym(handle, "Pi");

            error = dlerror();
            if (error != NULL) {
                fprintf(stderr, "Error locating symbols: %s\n", error);
                exit(EXIT_FAILURE);
            }

            printf("Switched to library %d\n", current_lib + 1);
        } else if (strcmp(cmd, "1") == 0) {
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

    dlclose(handle);
    return 0;
}
