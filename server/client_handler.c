#include "client_handler.h"
#include "server.h"

void broadcast(const char *msg, SOCKET exclude) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != exclude) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
}

void broadcast_file_header(const char *header, SOCKET exclude) {
    broadcast(header, exclude);
}

void broadcast_file_data(const char *data, int len, SOCKET exclude) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != exclude) {
            send(clients[i].socket, data, len, 0);
        }
    }
}

void handle_client(int index) {
    SOCKET client_socket = clients[index].socket;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            printf("[-] Client disconnected.\n");
            clients[index].active = 0;
            closesocket(client_socket);
            return;
        }

        // -----------------------------
        // PARSE PROTOCOL
        // -----------------------------
        if (strncmp(buffer, "LOGIN|", 6) == 0) {
            strcpy(clients[index].username, buffer + 6);

            char msg[100];
            snprintf(msg, sizeof(msg), "[%s joined the chat]", clients[index].username);
            broadcast(msg, client_socket);
            continue;
        }

        if (strncmp(buffer, "FILE|", 5) == 0) {
            printf("[FILE HEADER] %s\n", buffer);
            broadcast_file_header(buffer, client_socket);
            continue;
        }

        if (strncmp(buffer, "FILE_END", 8) == 0) {
            broadcast("FILE_END", client_socket);
            continue;
        }

        // FILE DATA (binary chunks)
        if (strstr(buffer, "FILE|") == NULL && strstr(buffer, "FILE_END") == NULL) {
            // Treat as standard message
            char msg[2100];
            snprintf(msg, sizeof(msg), "%s: %s", clients[index].username, buffer);

            broadcast(msg, client_socket);
            printf("%s", msg);
        } else {
            // Forward file data
            broadcast_file_data(buffer, bytes, client_socket);
        }
    }
}
