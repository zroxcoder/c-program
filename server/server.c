#include "server.h"
#include "client_handler.h"

Client clients[MAX_CLIENTS];
int client_count = 0;

DWORD WINAPI client_thread(void *arg) {
    int index = *(int*)arg;
    handle_client(index);
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
    listen(server_socket, 10);

    printf("[SERVER] Started on port 4444...\n");

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
            continue;

        // add client
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = client_socket;
                clients[i].active = 1;
                strcpy(clients[i].username, "Unknown");
                break;
            }
        }

        printf("[+] Client connected. Index=%d\n", i);

        int *index = malloc(sizeof(int));
        *index = i;

        HANDLE thread = CreateThread(NULL, 0, client_thread, index, 0, NULL);
        CloseHandle(thread);
    }

    closesocket(server_socket);
    WSACleanup();
}
