#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

void handle_client(SOCKET client_socket);

#endif
