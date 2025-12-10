#include "server.h"
#include "client_handler.h"

SOCKET clients[MAX_CLIENTS];
int client_count = 0;

DWORD WINAPI client_thread(void *sock) {
    SOCKET client_socket = *(SOCKET*)sock;
    handle_client(client_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4444);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Server running...\n");

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);

        printf("Client connected.\n");

        clients[client_count++] = client_socket;

        HANDLE thread = CreateThread(NULL, 0, client_thread, &client_socket, 0, NULL);
        CloseHandle(thread);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
