#include <stdio.h>
#include <math.h>
#include <dlfcn.h>

typedef double (*DerivativeFunc)(double, double);

void test_derivative(const char* lib_path, const char* impl_name) {
    void* handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading %s: %s\n", lib_path, dlerror());
        return;
    }

    DerivativeFunc Derivative = (DerivativeFunc)dlsym(handle, "Derivative");
    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error locating Derivative in %s: %s\n", lib_path, error);
        dlclose(handle);
        return;
    }

    double A = 0.5;
    double deltaX = 0.0001;
    double result = Derivative(A, deltaX);
    double expected = -sin(A);
    double difference = fabs(result - expected);

    printf("Testing Derivative (%s):\n", impl_name);
    printf("Computed derivative: %.10f\n", result);
    printf("Expected derivative: %.10f\n", expected);
    printf("Difference: %.10f\n\n", difference);

    dlclose(handle);
}

int main() {
    test_derivative("./libimpl1.so", "Implementation 1");
    test_derivative("./libimpl2.so", "Implementation 2");
    return 0;
}
