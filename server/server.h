#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 4096
#define SERVER_PORT 4444
#define MAX_CHANNELS 32
#define HISTORY_COUNT 50

typedef struct {
    char name[64];
    char password[64]; // empty = public
    int user_count;
    char history[HISTORY_COUNT][BUFFER_SIZE];
    int history_count;
} Channel;

typedef struct {
    SOCKET socket;
    char username[64];
    char channel[64];
    int active;
    int muted;
    DWORD threadId;
} Client;

extern Client clients[MAX_CLIENTS];
extern int client_count;

extern Channel channels[MAX_CHANNELS];
extern int channel_count;

void log_event(const char *msg);
void make_timestamp(char *out);

#endif
