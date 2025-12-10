#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void handle_client(int client_index);

// Timestamp
void make_timestamp(char *out);

// Channel broadcasting
void broadcast_channel(const char *msg, const char *channel, SOCKET exclude);

// Private message
void send_private(int index, const char *target, const char *msg);

// Kick user
void kick_user(const char *username);

#endif
