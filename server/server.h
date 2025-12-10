#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

extern SOCKET clients[MAX_CLIENTS];
extern int client_count;

DWORD WINAPI client_thread(void *socket);

#endif
