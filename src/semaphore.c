#include <string.h>
#include "semaphore.h"

int create_semaphore(const char *name, int initial_value) {
    if (kernel.semaphore_count >= MAX_SEMAPHORES) return -1;

    semaphore_t *sem = &kernel.semaphores[kernel.semaphore_count];
    sem->id = kernel.semaphore_count;
    sem->value = initial_value;
    sem->wait_count = 0;
    strncpy(sem->name, name, 31);
    sem->name[31] = '\0';

    return kernel.semaphore_count++;
}

void sem_wait(int sem_id) {
    if (sem_id < 0 || sem_id >= kernel.semaphore_count) return;

    semaphore_t *sem = &kernel.semaphores[sem_id];
    sem->value--;

    if (sem->value < 0) {
        int pid = kernel.current_pid;
        sem->waiting_procs[sem->wait_count++] = pid;
        kernel.processes[pid].state = PROC_BLOCKED;
        kernel.processes[pid].blocked_on_sem = sem_id;
    }
}

void sem_signal(int sem_id) {
    if (sem_id < 0 || sem_id >= kernel.semaphore_count) return;

    semaphore_t *sem = &kernel.semaphores[sem_id];
    sem->value++;

    if (sem->wait_count > 0) {
        int wakeup_pid = sem->waiting_procs[0];
        for (int i = 0; i < sem->wait_count - 1; i++) {
            sem->waiting_procs[i] = sem->waiting_procs[i + 1];
        }
        sem->wait_count--;

        kernel.processes[wakeup_pid].state = PROC_READY;
        kernel.processes[wakeup_pid].blocked_on_sem = -1;
    }
}
