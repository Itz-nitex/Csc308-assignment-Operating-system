#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10
#define ITERATIONS 100000

// Shared counter
int counter = 0;
pthread_mutex_t mutex;

// Thread function
void* increment_with_mutex(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    
    printf("Thread %d completed\n", thread_id);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, increment_with_mutex, &thread_ids[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print result
    printf("\nExpected counter: %d\n", NUM_THREADS * ITERATIONS);
    printf("Actual counter with mutex: %d\n", counter);
    printf("Difference: %d\n", NUM_THREADS * ITERATIONS - counter);
    
    // Clean up
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
