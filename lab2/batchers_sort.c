#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define MAX_PRINT_SIZE 16

typedef struct {
    int *array;
    int n;
    int numThreads;
    int threadId;
} THREAD_DATA;

CRITICAL_SECTION printLock;
SYNCHRONIZATION_BARRIER barrier;

void PrintArray(int *array, int n) {
    printf("[");
    for (int i = 0; i < n; i++) {
        printf("%d", array[i]);
        if (i < n - 1) printf(", ");
    }
    printf("]\n");
}

DWORD WINAPI ThreadFunc(LPVOID param) {
    THREAD_DATA *data = (THREAD_DATA *)param;
    int *array = data->array;
    int n = data->n;
    int numThreads = data->numThreads;
    int threadId = data->threadId;

    EnterCriticalSection(&printLock);
    printf("Thread %d created. Total threads: %d\n", threadId, numThreads);
    LeaveCriticalSection(&printLock);

    for (int size = 2; size <= n; size *= 2) {
        for (int subSize = size / 2; subSize > 0; subSize /= 2) {
            int step = size / subSize;
            for (int i = threadId; i < n; i += numThreads) {
                int j = i + subSize;
                if (j < n && (i / size == j / size)) {
                    if (array[i] > array[j]) {
                        int temp = array[i];
                        array[i] = array[j];
                        array[j] = temp;
                    }
                }
            }
            EnterSynchronizationBarrier(&barrier, SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE);
        }
    }
    return 0;
}

int IsSorted(int *array, int n) {
    for (int i = 1; i < n; i++) {
        if (array[i - 1] > array[i]) return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_threads>\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[1]);
    if (numThreads <= 0) {
        printf("Number of threads must be a positive integer.\n");
        return 1;
    }

    int n = 1 << 16;
    int *array = (int *)malloc(n * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation error\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    for (int i = 0; i < n; i++) {
        array[i] = rand();
    }

    printf("First %d elements of the initial array:\n", MAX_PRINT_SIZE);
    PrintArray(array, MAX_PRINT_SIZE);
    printf("Last %d elements of the initial array:\n", MAX_PRINT_SIZE);
    PrintArray(array + n - MAX_PRINT_SIZE, MAX_PRINT_SIZE);

    InitializeCriticalSection(&printLock);
    if (!InitializeSynchronizationBarrier(&barrier, numThreads, -1)) {
        printf("Failed to initialize synchronization barrier\n");
        return 1;
    }

    HANDLE *threads = (HANDLE *)malloc(numThreads * sizeof(HANDLE));
    THREAD_DATA *threadData = (THREAD_DATA *)malloc(numThreads * sizeof(THREAD_DATA));

    clock_t start = clock();

    for (int i = 0; i < numThreads; i++) {
        threadData[i].array = array;
        threadData[i].n = n;
        threadData[i].numThreads = numThreads;
        threadData[i].threadId = i;

        threads[i] = CreateThread(NULL, 0, ThreadFunc, &threadData[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
        getchar();
    }

    WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

    clock_t end = clock();

    double timeSpent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Sorting time: %f seconds\n", timeSpent);

    if (IsSorted(array, n)) {
        printf("Array is sorted correctly.\n");
    } else {
        printf("Sorting error.\n");
    }

    printf("First %d elements of the sorted array:\n", MAX_PRINT_SIZE);
    PrintArray(array, MAX_PRINT_SIZE);
    printf("Last %d elements of the sorted array:\n", MAX_PRINT_SIZE);
    PrintArray(array + n - MAX_PRINT_SIZE, MAX_PRINT_SIZE);

    for (int i = 0; i < numThreads; i++) {
        CloseHandle(threads[i]);
    }
    free(threads);
    free(threadData);
    free(array);

    DeleteCriticalSection(&printLock);
    DeleteSynchronizationBarrier(&barrier);

    return 0;
}
