#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 10
#define ITERATIONS 100000

// Shared counter
int counter = 0;

// For counting semaphore experiment
#define MAX_CONCURRENT 3
sem_t counting_sem;

// For mutex comparison
pthread_mutex_t mutex;

// Using binary semaphore
void* increment_with_semaphore(void* arg) {
    sem_t* sem = (sem_t*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        sem_wait(sem);
        counter++;
        sem_post(sem);
    }
    
    return NULL;
}

// Using mutex
void* increment_with_mutex(void* arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    
    return NULL;
}

// Using counting semaphore (allows multiple threads)
void* increment_with_counting_semaphore(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < ITERATIONS / 10; i++) {
        sem_wait(&counting_sem);  // Wait if max concurrent threads reached
        counter++;
        printf("Thread %d is executing\n", thread_id);
        usleep(1000);  // Simulate work
        sem_post(&counting_sem);
    }
    
    return NULL;
}

void test_semaphore() {
    sem_t sem;
    sem_init(&sem, 0, 1);
    
    pthread_t threads[NUM_THREADS];
    counter = 0;
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_with_semaphore, &sem);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Semaphore: Counter = %d, Expected = %d\n", 
           counter, NUM_THREADS * ITERATIONS);
    printf("Semaphore Time: %.4f seconds\n\n", time_spent);
    
    sem_destroy(&sem);
}

void test_mutex() {
    pthread_t threads[NUM_THREADS];
    counter = 0;
    
    pthread_mutex_init(&mutex, NULL);
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_with_mutex, NULL);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Mutex: Counter = %d, Expected = %d\n", 
           counter, NUM_THREADS * ITERATIONS);
    printf("Mutex Time: %.4f seconds\n\n", time_spent);
    
    pthread_mutex_destroy(&mutex);
}

void test_counting_semaphore() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    counter = 0;
    
    sem_init(&counting_sem, 0, MAX_CONCURRENT);
    
    printf("Testing counting semaphore - Max %d threads concurrently\n", 
           MAX_CONCURRENT);
    printf("Each thread will execute %d iterations\n\n", ITERATIONS / 10);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, increment_with_counting_semaphore, 
                      &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nCounting Semaphore: Counter = %d\n", counter);
    printf("Expected = %d\n\n", NUM_THREADS * (ITERATIONS / 10));
    
    sem_destroy(&counting_sem);
}

int main() {
    printf("=== Semaphore vs Mutex Comparison ===\n\n");
    
    test_semaphore();
    test_mutex();
    
    printf("=== Counting Semaphore Demonstration ===\n");
    test_counting_semaphore();
    
    printf("\nKey Takeaways:\n");
    printf("- Mutex: Simpler, faster for binary exclusion\n");
    printf("- Semaphore: More flexible, can control multiple resources\n");
    printf("- Counting semaphore: Ideal for resource pools (connection pools, etc.)\n");
    printf("- Use mutex for simple mutual exclusion, semaphore for resource management\n");
    
    return 0;
}
