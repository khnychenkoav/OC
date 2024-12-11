#include <stdio.h>
#include <math.h>
#include <dlfcn.h>

typedef double (*PiFunc)(int);

void test_pi(const char* lib_path, const char* impl_name) {
    void* handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading %s: %s\n", lib_path, dlerror());
        return;
    }

    PiFunc Pi = (PiFunc)dlsym(handle, "Pi");
    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error locating Pi in %s: %s\n", lib_path, error);
        dlclose(handle);
        return;
    }

    int K = 1000000;
    double result = Pi(K);
    double expected = M_PI;
    double difference = fabs(result - expected);

    printf("Testing Pi (%s):\n", impl_name);
    printf("Computed Pi: %.15f\n", result);
    printf("Expected Pi: %.15f\n", expected);
    printf("Difference: %.15f\n\n", difference);

    dlclose(handle);
}

int main() {
    test_pi("./libimpl1.so", "Implementation 1");
    test_pi("./libimpl2.so", "Implementation 2");
    return 0;
}
