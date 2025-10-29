#ifndef IPC_H
#define IPC_H

#include "kernel.h"

void init_msg_queue(msg_queue_t *q);
int send_message(int sender_pid, int receiver_pid, const char *data, size_t size, int msg_type);
int receive_message(int pid, message_t *msg, int blocking);
int broadcast_message(int sender_pid, const char *data, size_t size);

void sys_send(int receiver_pid, const char *data, size_t size, int msg_type);
int sys_receive(message_t *msg, int blocking);
int sys_broadcast(const char *data, size_t size);

#endif
