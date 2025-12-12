#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include "server.h"

// Send / receive framed messages
int send_msg_sock(SOCKET s, const void *buf, uint32_t len);
int recv_msg_sock(SOCKET s, char **out, uint32_t *out_len);

// Thread wrapper declaration
DWORD WINAPI client_thread_wrapper(LPVOID arg);

// Client handling
void broadcast_channel(const char *msg, const char *channel, SOCKET exclude);
void send_private(int index, const char *target, const char *msg);
void handle_client(int index);

#endif
