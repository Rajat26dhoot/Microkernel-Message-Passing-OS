#include <stdio.h>
#include <string.h>
#include "ipc.h"
#include "scheduler.h"

void init_msg_queue(msg_queue_t *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

int send_message(int sender_pid, int receiver_pid, const char *data,
                 size_t size, int msg_type) {
    if (receiver_pid < 0 || receiver_pid >= MAX_PROCESSES) return -1;
    if (kernel.processes[receiver_pid].state == PROC_TERMINATED) return -1;

    msg_queue_t *q = &kernel.processes[receiver_pid].msg_queue;

    if (q->count >= MAX_MESSAGES) {
        printf("[IPC] Warning: Message queue full for PID %d\n", receiver_pid);
        return -1;
    }

    message_t *msg = &q->messages[q->tail];
    msg->sender_pid = sender_pid;
    msg->receiver_pid = receiver_pid;
    msg->msg_type = msg_type;
    msg->size = size < MESSAGE_SIZE ? size : MESSAGE_SIZE;
    msg->timestamp = get_time_ns();
    memcpy(msg->data, data, msg->size);

    q->tail = (q->tail + 1) % MAX_MESSAGES;
    q->count++;

    kernel.processes[sender_pid].messages_sent++;

    if (kernel.processes[receiver_pid].state == PROC_BLOCKED ||
        kernel.processes[receiver_pid].state == PROC_WAITING) {
        kernel.processes[receiver_pid].state = PROC_READY;
    }

    return 0;
}

int receive_message(int pid, message_t *msg, int blocking) {
    msg_queue_t *q = &kernel.processes[pid].msg_queue;

    if (q->count == 0) {
        if (blocking) {
            kernel.processes[pid].state = PROC_WAITING;
        }
        return -1;
    }

    *msg = q->messages[q->head];
    q->head = (q->head + 1) % MAX_MESSAGES;
    q->count--;

    kernel.processes[pid].messages_received++;

    return 0;
}

int broadcast_message(int sender_pid, const char *data, size_t size) {
    int sent = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (i != sender_pid && kernel.processes[i].state != PROC_TERMINATED &&
            kernel.processes[i].pid != -1) {
            if (send_message(sender_pid, i, data, size, MSG_BROADCAST) == 0) {
                sent++;
            }
        }
    }
    return sent;
}

void sys_send(int receiver_pid, const char *data, size_t size, int msg_type) {
    send_message(kernel.current_pid, receiver_pid, data, size, msg_type);
}

int sys_receive(message_t *msg, int blocking) {
    return receive_message(kernel.current_pid, msg, blocking);
}

int sys_broadcast(const char *data, size_t size) {
    return broadcast_message(kernel.current_pid, data, size);
}
