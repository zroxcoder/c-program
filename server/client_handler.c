#include "client_handler.h"
#include "server.h"

// Timestamp generator
void make_timestamp(char *out) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(out, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
}

// Send message to all in same channel
void broadcast_channel(const char *msg, const char *channel, SOCKET exclude) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].channel, channel) == 0 && clients[i].socket != exclude) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
}

// PRIVATE MESSAGE
void send_private(int index, const char *target, const char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, target) == 0) {
            char buffer[300];
            sprintf(buffer, "[PM from %s] %s", clients[index].username, msg);
            send(clients[i].socket, buffer, strlen(buffer), 0);
            return;
        }
    }
}

// KICK USER
void kick_user(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].username, username) == 0) {
            send(clients[i].socket, "[SERVER] You were kicked.", 30, 0);
            closesocket(clients[i].socket);
            clients[i].active = 0;

            char logmsg[100];
            sprintf(logmsg, "Kicked user: %s", username);
            log_event(logmsg);
        }
    }
}

// Handle client messages
void handle_client(int index) {
    SOCKET sock = clients[index].socket;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

        if (bytes <= 0) {
            clients[index].active = 0;
            closesocket(sock);
            return;
        }

        // LOGIN
        if (strncmp(buffer, "LOGIN|", 6) == 0) {
            strcpy(clients[index].username, buffer + 6);
            continue;
        }

        // PRIVATE MESSAGE
        if (strncmp(buffer, "/pm ", 4) == 0) {
            char *target = strtok(buffer + 4, " ");
            char *message = strtok(NULL, "\0");
            send_private(index, target, message);
            continue;
        }

        // JOIN CHANNEL
        if (strncmp(buffer, "/join ", 6) == 0) {
            strcpy(clients[index].channel, buffer + 6);
            continue;
        }

        // USER LIST
        if (strncmp(buffer, "/users", 6) == 0) {
            char list[500] = "Users:\n";
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active) {
                    strcat(list, clients[i].username);
                    strcat(list, "\n");
                }
            }
            send(sock, list, strlen(list), 0);
            continue;
        }

        // Normal message with timestamp
        char ts[20], msg[2200];
        make_timestamp(ts);
        sprintf(msg, "[%s] %s: %s", ts, clients[index].username, buffer);

        broadcast_channel(msg, clients[index].channel, sock);
    }
}
