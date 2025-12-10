#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "192.168.1.71"
#define SERVER_PORT 4444

SOCKET sock;

// ---------------------
// SAVE RECEIVED FILE
// ---------------------
void save_file(const char *filename, const char *data, int len) {
    char path[200];
    sprintf(path, "downloads/%s", filename);

    FILE *fp = fopen(path, "ab");
    if (fp) {
        fwrite(data, 1, len, fp);
        fclose(fp);
    }
}

// ---------------------
// RECEIVE THREAD
// ---------------------
unsigned __stdcall receive_messages(void *arg) {
    char buffer[2048];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);

        if (bytes <= 0) exit(0);

        printf("\n%s\n> ", buffer);
        fflush(stdout);
    }
    return 0;
}

// ---------------------
// SEND FILE
// ---------------------
void send_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("Cannot open file.\n");
        return;
    }

    char header[200];
    const char *name = strrchr(path, '\\');
    if (!name) name = strrchr(path, '/');
    if (!name) name = path; else name++;

    sprintf(header, "FILE|%s", name);
    send(sock, header, strlen(header), 0);
    Sleep(100);

    char chunk[1024];
    int b;

    while ((b = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        send(sock, chunk, b, 0);
    }

    send(sock, "FILE_END", 8, 0);
    fclose(fp);
}

// ---------------------
// MAIN
// ---------------------
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    connect(sock, (struct sockaddr*)&server, sizeof(server));
    printf("Connected to server.\n");

    char username[50];
    printf("Username: ");
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = 0;

    char pkt[100];
    sprintf(pkt, "LOGIN|%s", username);
    send(sock, pkt, strlen(pkt), 0);

    unsigned id;
    HANDLE t = (HANDLE)_beginthreadex(NULL, 0, receive_messages, NULL, 0, &id);
    CloseHandle(t);

    char input[1024];
    while (1) {
        printf("> ");
        fgets(input, 1024, stdin);

        if (strncmp(input, "/file ", 6) == 0) {
            input[strcspn(input, "\n")] = 0;
            send_file(input + 6);
            continue;
        }

        send(sock, input, strlen(input), 0);
    }
}
