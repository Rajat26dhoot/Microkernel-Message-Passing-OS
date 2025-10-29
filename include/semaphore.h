#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "kernel.h"

int create_semaphore(const char *name, int initial_value);
void sem_wait(int sem_id);
void sem_signal(int sem_id);

#endif
