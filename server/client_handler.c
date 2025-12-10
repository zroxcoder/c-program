#include "client_handler.h"
#include "server.h"

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (bytes <= 0) {
            printf("Client disconnected.\n");
            closesocket(client_socket);
            return;
        }

        printf("Client: %s", buffer);

        // Broadcast
        for (int i = 0; i < client_count; i++) {
            if (clients[i] != client_socket) {
                send(clients[i], buffer, bytes, 0);
            }
        }
    }
}
