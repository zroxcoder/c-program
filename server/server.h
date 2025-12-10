#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    SOCKET socket;
    char username[50];
    int active;
} Client;

extern Client clients[MAX_CLIENTS];
extern int client_count;

DWORD WINAPI client_thread(void *socket);

void broadcast(const char *msg, SOCKET exclude);

void broadcast_file_header(const char *header, SOCKET exclude);
void broadcast_file_data(const char *data, int len, SOCKET exclude);

#endif
