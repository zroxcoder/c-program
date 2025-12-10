#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define SERVER_PORT 4444

// -----------------------------------
// CLIENT STRUCT
// -----------------------------------
typedef struct {
    SOCKET socket;
    char username[50];
    char channel[50];
    int active;
} Client;

extern Client clients[MAX_CLIENTS];
extern int client_count;

// -----------------------------------
// FEATURE FUNCTIONS
// -----------------------------------
void broadcast_channel(const char *msg, const char *channel, SOCKET exclude);
void send_private(int index, const char *target, const char *msg);
void kick_user(const char *username);

// -----------------------------------
// ADMIN & LOGGING
// -----------------------------------
void create_admin_console();
void log_event(const char *msg);

#endif
