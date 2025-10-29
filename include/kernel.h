#ifndef KERNEL_H
#define KERNEL_H

#include <ucontext.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_PROCESSES 32
#define STACK_SIZE (1024 * 64)
#define MAX_MESSAGES 64
#define MESSAGE_SIZE 512
#define MAX_SEMAPHORES 16
#define PROCESS_NAME_LEN 32
#define TIME_QUANTUM 10000000

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_WAITING,
    PROC_TERMINATED
} proc_state_t;

typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} priority_t;

typedef enum {
    MSG_NORMAL = 0,
    MSG_REQUEST = 1,
    MSG_REPLY = 2,
    MSG_BROADCAST = 3,
    MSG_SIGNAL = 4
} msg_type_t;

typedef struct {
    int sender_pid;
    int receiver_pid;
    int msg_type;
    char data[MESSAGE_SIZE];
    size_t size;
    uint64_t timestamp;
} message_t;

typedef struct {
    message_t messages[MAX_MESSAGES];
    int head;
    int tail;
    int count;
} msg_queue_t;

typedef struct {
    int value;
    int waiting_procs[MAX_PROCESSES];
    int wait_count;
    int id;
    char name[32];
} semaphore_t;

typedef struct {
    int pid;
    char name[PROCESS_NAME_LEN];
    proc_state_t state;
    ucontext_t context;
    char stack[STACK_SIZE];
    msg_queue_t msg_queue;
    priority_t priority;
    uint64_t cpu_time;
    uint64_t start_time;
    uint64_t wait_time;
    int blocked_on_sem;
    int quantum_remaining;
    int messages_sent;
    int messages_received;
} pcb_t;

typedef struct {
    pcb_t processes[MAX_PROCESSES];
    int current_pid;
    int process_count;
    ucontext_t scheduler_context;
    uint64_t context_switches;
    uint64_t total_switch_time_ns;
    uint64_t system_time;
    semaphore_t semaphores[MAX_SEMAPHORES];
    int semaphore_count;
    int test_case_running;
} kernel_t;

extern kernel_t kernel;

uint64_t get_time_ns();
const char* state_to_string(proc_state_t state);
const char* priority_to_string(priority_t priority);

#endif
