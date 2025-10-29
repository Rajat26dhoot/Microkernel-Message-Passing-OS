#include <stdio.h>
#include <string.h>
#include "process.h"
#include "scheduler.h"
#include "ipc.h"


int create_process(void (*func)(void), const char *name, priority_t priority) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (kernel.processes[i].state == PROC_TERMINATED ||
            kernel.processes[i].pid == -1) {

            pcb_t *proc = &kernel.processes[i];
            proc->pid = i;
            strncpy(proc->name, name, PROCESS_NAME_LEN - 1);
            proc->name[PROCESS_NAME_LEN - 1] = '\0';
            proc->state = PROC_READY;
            proc->priority = priority;
            proc->cpu_time = 0;
            proc->start_time = get_time_ns();
            proc->wait_time = 0;
            proc->blocked_on_sem = -1;
            proc->messages_sent = 0;
            proc->messages_received = 0;
            init_msg_queue(&proc->msg_queue);

            getcontext(&proc->context);
            proc->context.uc_stack.ss_sp = proc->stack;
            proc->context.uc_stack.ss_size = STACK_SIZE;
            proc->context.uc_link = &kernel.scheduler_context;
            makecontext(&proc->context, func, 0);

            kernel.process_count++;
            return i;
        }
    }
    return -1;
}

void init_kernel() {
    memset(&kernel, 0, sizeof(kernel));
    kernel.current_pid = -1;
    kernel.process_count = 0;
    kernel.context_switches = 0;
    kernel.total_switch_time_ns = 0;
    kernel.system_time = 0;
    kernel.semaphore_count = 0;
    kernel.test_case_running = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        kernel.processes[i].pid = -1;
        kernel.processes[i].state = PROC_TERMINATED;
    }

    getcontext(&kernel.scheduler_context);
}
