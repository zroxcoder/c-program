#include "server.h"
#include "client_handler.h"

Client clients[MAX_CLIENTS];
int client_count = 0;
FILE *log_file;

// ----------------------------
// LOGGING SYSTEM
// ----------------------------
void log_event(const char *msg) {
    log_file = fopen("server_logs.txt", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", msg);
        fclose(log_file);
    }
}

// ----------------------------
// MAIN SERVER LOOP
// ----------------------------
DWORD WINAPI client_thread(void *arg) {
    int index = *(int*)arg;
    free(arg);
    handle_client(index);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    printf("[SERVER] Running on port %d...\n", SERVER_PORT);
    log_event("SERVER STARTED");

    // ----------------------------
    // ADMIN CONSOLE THREAD
    // ----------------------------
    create_admin_console();

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
            continue;

        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = client_socket;
                clients[i].active = 1;
                strcpy(clients[i].username, "Unknown");
                strcpy(clients[i].channel, "general");
                break;
            }
        }

        printf("[+] Client connected. Index=%d\n", i);

        int *client_index = malloc(sizeof(int));
        *client_index = i;

        HANDLE t = CreateThread(NULL, 0, client_thread, client_index, 0, NULL);
        CloseHandle(t);
    }

    closesocket(server_socket);
    WSACleanup();
}
