#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define SHM_SIZE 1024
#define SEM_NAME "/shared_mem_sem"
#define NUM_MESSAGES 5

// Structure to organize shared memory data
typedef struct {
    char message[SHM_SIZE];
    int message_count;
    int producer_done;
    int consumer_ready;
} shared_data_t;

// Function prototypes
void producer_process(shared_data_t *shared_mem, sem_t *sem);
void consumer_process(shared_data_t *shared_mem, sem_t *sem);
void cleanup_resources(int shmid, shared_data_t *shared_mem, sem_t *sem);

int main() {
    int shmid;
    key_t key;
    shared_data_t *shared_mem;
    sem_t *sem;
    pid_t pid;
    
    // Generate unique key for shared memory
    key = ftok("/tmp", 'A');
    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }
    
    // Create shared memory segment
    shmid = shmget(key, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    
    // Attach shared memory to process address space
    shared_mem = (shared_data_t *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    
    // Initialize shared memory
    memset(shared_mem->message, 0, SHM_SIZE);
    shared_mem->message_count = 0;
    shared_mem->producer_done = 0;
    shared_mem->consumer_ready = 0;
    
    // Create named semaphore for synchronization
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            // Semaphore already exists, open it
            sem = sem_open(SEM_NAME, 0);
            if (sem == SEM_FAILED) {
                perror("sem_open failed");
                cleanup_resources(shmid, shared_mem, NULL);
                exit(EXIT_FAILURE);
            }
            // Remove the existing semaphore so we can recreate it
            sem_unlink(SEM_NAME);
            sem_close(sem);
            // Recreate with proper initialization
            sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
            if (sem == SEM_FAILED) {
                perror("sem_open recreate failed");
                cleanup_resources(shmid, shared_mem, NULL);
                exit(EXIT_FAILURE);
            }
        } else {
            perror("sem_open failed");
            cleanup_resources(shmid, shared_mem, NULL);
            exit(EXIT_FAILURE);
        }
    }
    
    printf("Shared memory created with ID: %d\n", shmid);
    printf("Semaphore initialized\n\n");
    
    // Fork the process
    pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        cleanup_resources(shmid, shared_mem, sem);
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Child process - Consumer
        printf("Consumer process (PID: %d) started\n", getpid());
        consumer_process(shared_mem, sem);
        printf("Consumer process finished\n");
    } else {
        // Parent process - Producer
        printf("Producer process (PID: %d) started\n", getpid());
        producer_process(shared_mem, sem);
        printf("Producer process finished\n");
        
        // Wait for child to finish
        wait(NULL);
        
        // Clean up
        cleanup_resources(shmid, shared_mem, sem);
    }
    
    return 0;
}

void producer_process(shared_data_t *shared_mem, sem_t *sem) {
    char messages[NUM_MESSAGES][50] = {
        "Hello from Producer!",
        "This is message #2",
        "Shared memory is fast!",
        "IPC without networking",
        "Producer says: Done!"
    };
    
    printf("\n=== Producer: Starting to send messages ===\n");
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        // Wait for semaphore (critical section)
        sem_wait(sem);
        
        // Critical section: Write to shared memory
        strncpy(shared_mem->message, messages[i], SHM_SIZE - 1);
        shared_mem->message[SHM_SIZE - 1] = '\0';
        shared_mem->message_count = i + 1;
        
        printf("Producer: Wrote message %d: \"%s\"\n", i + 1, messages[i]);
        
        // Signal consumer that data is ready
        shared_mem->consumer_ready = 1;
        
        // Release semaphore
        sem_post(sem);
        
        // Simulate work
        sleep(1);
    }
    
    // Signal that producer is done
    sem_wait(sem);
    shared_mem->producer_done = 1;
    sem_post(sem);
    
    printf("Producer: All messages sent\n");
}

void consumer_process(shared_data_t *shared_mem, sem_t *sem) {
    int last_message_count = 0;
    int messages_consumed = 0;
    
    printf("\n=== Consumer: Starting to receive messages ===\n");
    
    while (1) {
        // Wait for semaphore
        sem_wait(sem);
        
        // Check if there's new data
        if (shared_mem->consumer_ready && 
            shared_mem->message_count > last_message_count) {
            
            // Read from shared memory
            printf("Consumer: Read message %d: \"%s\"\n", 
                   shared_mem->message_count, shared_mem->message);
            
            last_message_count = shared_mem->message_count;
            messages_consumed++;
            shared_mem->consumer_ready = 0;
            
            sem_post(sem);
            
            // Check if producer is done
            if (shared_mem->producer_done && 
                messages_consumed >= shared_mem->message_count) {
                printf("Consumer: All messages received\n");
                break;
            }
        } else {
            sem_post(sem);
            
            // Check if producer is done and no more messages
            if (shared_mem->producer_done && 
                messages_consumed >= shared_mem->message_count) {
                printf("Consumer: All messages received\n");
                break;
            }
            
            // Small delay to avoid busy waiting
            usleep(100000); // 100ms
        }
    }
}

void cleanup_resources(int shmid, shared_data_t *shared_mem, sem_t *sem) {
    printf("\n=== Cleaning up resources ===\n");
    
    // Detach shared memory
    if (shmdt(shared_mem) == -1) {
        perror("shmdt failed");
    } else {
        printf("Shared memory detached\n");
    }
    
    // Remove shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    } else {
        printf("Shared memory removed\n");
    }
    
    // Close and unlink semaphore
    if (sem != NULL) {
        if (sem_close(sem) == -1) {
            perror("sem_close failed");
        } else {
            printf("Semaphore closed\n");
        }
        
        if (sem_unlink(SEM_NAME) == -1) {
            perror("sem_unlink failed");
        } else {
            printf("Semaphore unlinked\n");
        }
    }
}
