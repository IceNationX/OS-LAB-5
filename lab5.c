/* 
Mohammad Al-Lozy, 100829387
Faisal Akbar, 100846786
Alexy Pichette, 100822470
CRN: 74025
*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_PROCESSES 5
#define NUM_RESOURCES 3
#define MAX_ITERATIONS 5  // Limit number of iterations per process

// Resource management structures
int available_resources[NUM_RESOURCES];
int allocated_resources[NUM_PROCESSES][NUM_RESOURCES] = {
    {0, 1, 0},  // P0
    {2, 0, 0},  // P1
    {3, 0, 2},  // P2
    {2, 1, 1},  // P3
    {0, 0, 2}   // P4
};

int maximum_resources[NUM_PROCESSES][NUM_RESOURCES] = {
    {7, 5, 3},  // P0
    {3, 2, 2},  // P1
    {9, 0, 2},  // P2
    {2, 2, 2},  // P3
    {4, 3, 3}   // P4
};

int remaining_resources[NUM_PROCESSES][NUM_RESOURCES];
pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;

void computeRemainingResources() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            remaining_resources[i][j] = maximum_resources[i][j] - allocated_resources[i][j];
        }
    }
}

// Print current system state
void printSystemState() {
    printf("\n🔍 Available Resources:\n");
    for (int i = 0; i < NUM_RESOURCES; i++) {
        printf("%d ", available_resources[i]);
    }
    printf("\n");

    printf("\n🔍 Allocated Resources:\n");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            printf("%d ", allocated_resources[i][j]);
        }
        printf("\n");
    }

    printf("\n🔍 Remaining Resources (Need):\n");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            printf("%d ", remaining_resources[i][j]);
        }
        printf("\n");
    }
}

// Check if the system is in a safe state
bool isSystemSafe() {
    int work[NUM_RESOURCES];
    bool completed[NUM_PROCESSES] = {false};
    int safe_sequence[NUM_PROCESSES];
    int count = 0;

    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = available_resources[i];
    }

    while (count < NUM_PROCESSES) {
        bool found = false;
        for (int p = 0; p < NUM_PROCESSES; p++) {
            if (!completed[p]) {
                int j;
                for (j = 0; j < NUM_RESOURCES; j++) {
                    if (remaining_resources[p][j] > work[j])
                        break;
                }
                if (j == NUM_RESOURCES) {
                    for (int k = 0; k < NUM_RESOURCES; k++) {
                        work[k] += allocated_resources[p][k];
                    }
                    safe_sequence[count++] = p;
                    completed[p] = true;
                    found = true;
                }
            }
        }
        if (!found) {
            printf("\n❌ No safe sequence found. System is in an unsafe state!\n");
            return false;
        }
    }

    printf("\n✅ System is in a safe state. Safe sequence: ");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("P%d ", safe_sequence[i]);
    }
    printf("\n");
    return true;
}

// Request resources from a process
int requestResources(int processID, int requested[]) {
    pthread_mutex_lock(&resource_mutex);

    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (requested[i] > remaining_resources[processID][i] || requested[i] > available_resources[i]) {
            printf("❌ Process P%d requested too many resources.\n", processID);
            pthread_mutex_unlock(&resource_mutex);
            return -1;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        available_resources[i] -= requested[i];
        allocated_resources[processID][i] += requested[i];
        remaining_resources[processID][i] -= requested[i];
    }

    if (!isSystemSafe()) {
        for (int i = 0; i < NUM_RESOURCES; i++) {
            available_resources[i] += requested[i];
            allocated_resources[processID][i] -= requested[i];
            remaining_resources[processID][i] += requested[i];
        }
        printf("⚠️ Request denied. Unsafe state detected.\n");
        pthread_mutex_unlock(&resource_mutex);
        return -1;
    }

    printf("✅ Process P%d granted resources.\n", processID);
    pthread_mutex_unlock(&resource_mutex);
    return 0;
}

// Release resources back to the system
void releaseResources(int processID, int released[]) {
    pthread_mutex_lock(&resource_mutex);
    for (int i = 0; i < NUM_RESOURCES; i++) {
        allocated_resources[processID][i] -= released[i];
        available_resources[i] += released[i];
        remaining_resources[processID][i] += released[i];
    }
    printf("🔄 Process P%d released resources.\n", processID);
    pthread_mutex_unlock(&resource_mutex);
}

// Simulate process execution
void *processExecution(void *arg) {
    int processID = *(int *)arg;
    free(arg);

    for (int iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        sleep(rand() % 3 + 1);  // Simulate processing time.

        int requested[NUM_RESOURCES];
        for (int i = 0; i < NUM_RESOURCES; i++) {
            requested[i] = rand() % (remaining_resources[processID][i] + 1);
        }

        if (requestResources(processID, requested) == 0) {
            sleep(rand() % 3 + 1);  // Simulate execution with acquired resources.
            releaseResources(processID, requested);
        }

        printSystemState();  // Print system state after each iteration.
    }

    printf("✅ Process P%d has completed its iterations. Exiting...\n", processID);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <resource1> <resource2> <resource3>\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < NUM_RESOURCES; i++) {
        available_resources[i] = atoi(argv[i + 1]);
    }

    computeRemainingResources();

    if (!isSystemSafe()) {
        printf("❌ Initial state is unsafe. Exiting.\n");
        return 1;
    }

    printf("✅ System initialized in a safe state. Starting processes...\n");

    pthread_t threads[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int *processID = malloc(sizeof(*processID));
        if (processID == NULL) {
            perror("Failed to allocate memory for process ID");
            exit(EXIT_FAILURE);
        }
        *processID = i;
        pthread_create(&threads[i], NULL, processExecution, processID);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n🏁 All processes have completed. System shutting down.\n");
    return 0;
}

