#include "scheduler.h"

int find_next_process() {
    for (int prio = PRIORITY_REALTIME; prio >= PRIORITY_LOW; prio--) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            int pid = (kernel.current_pid + 1 + i) % MAX_PROCESSES;
            pcb_t *proc = &kernel.processes[pid];

            if (proc->state == PROC_READY && proc->priority == prio) {
                return pid;
            }
        }
    }
    return -1;
}

void sys_yield() {
    uint64_t start = get_time_ns();
    pcb_t *current = &kernel.processes[kernel.current_pid];
    current->state = PROC_READY;
    swapcontext(&current->context, &kernel.scheduler_context);
    uint64_t elapsed = get_time_ns() - start;
    kernel.total_switch_time_ns += elapsed;
}

void sys_sleep(int ms) {
    kernel.processes[kernel.current_pid].state = PROC_WAITING;
    sys_yield();
}

void sys_exit() {
    kernel.processes[kernel.current_pid].state = PROC_TERMINATED;
    swapcontext(&kernel.processes[kernel.current_pid].context,
                &kernel.scheduler_context);
}

void scheduler() {
    while (1) {
        int next_pid = find_next_process();

        if (next_pid == -1) {
            int active = 0;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (kernel.processes[i].state != PROC_TERMINATED &&
                    kernel.processes[i].pid != -1) {
                    active = 1;
                    break;
                }
            }
            if (!active) break;
            continue;
        }

        kernel.current_pid = next_pid;
        kernel.processes[next_pid].state = PROC_RUNNING;
        kernel.processes[next_pid].quantum_remaining = TIME_QUANTUM;
        kernel.context_switches++;

        uint64_t start = get_time_ns();
        swapcontext(&kernel.scheduler_context,
                    &kernel.processes[next_pid].context);
        uint64_t elapsed = get_time_ns() - start;

        kernel.processes[next_pid].cpu_time += elapsed;
        kernel.system_time += elapsed;
    }
}
