#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    setbuf(stdout, NULL);

    while (1) {
        char input[256];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            fprintf(stderr, "Failed to read input.\n");
            break;
        }
        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        int a, b;
        if (sscanf(input, "%d %d", &a, &b) == 2) {
            if (b == 0) {
                fprintf(stderr, "Division by zero. Terminating.\n");
                break;
            }
            double result = (double)a / b;
            printf("Result: %d / %d = %.6f\n", a, b, result);
        } else {
            fprintf(stderr, "Invalid input\n");
            break;
        }
    }

    return 0;
}
