#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10
#define ITERATIONS 100000

// Shared counter (no protection)
int counter = 0;

// Thread function without mutex
void* increment_without_mutex(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Race condition here - no synchronization
        counter++;
    }
    
    printf("Thread %d completed\n", thread_id);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, increment_without_mutex, &thread_ids[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print result
    printf("\nExpected counter: %d\n", NUM_THREADS * ITERATIONS);
    printf("Actual counter WITHOUT mutex: %d\n", counter);
    printf("Missing: %d\n", NUM_THREADS * ITERATIONS - counter);
    
    return 0;
}
