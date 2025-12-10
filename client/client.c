#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>   // for _beginthreadex

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "192.168.1.71"
#define SERVER_PORT 4444

SOCKET sock;

// -------------------------------
// RECEIVE THREAD
// -------------------------------
unsigned __stdcall receive_messages(void *arg) {
    char buffer[2048];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);

        if (bytes <= 0) {
            printf("\n[!] Disconnected from server.\n");
            exit(0);
        }

        printf("\n%s\n", buffer);
        printf("> ");
        fflush(stdout);
    }

    return 0;
}

// -------------------------------
// SEND FILE TO SERVER
// -------------------------------
void send_file(const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        printf("[X] Could not open file.\n");
        return;
    }

    // Extract filename from path
    const char *filename = strrchr(filepath, '\\');
    if (!filename) filename = filepath;
    else filename++;

    char header[512];
    snprintf(header, sizeof(header), "FILE|%s", filename);

    send(sock, header, strlen(header), 0);
    Sleep(100);

    printf("[+] Sending file: %s\n", filename);

    char chunk[1024];
    int bytes;

    while ((bytes = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        send(sock, chunk, bytes, 0);
        Sleep(5); // prevent packet flooding
    }

    send(sock, "FILE_END", 8, 0);
    fclose(fp);

    printf("[+] File transfer complete.\n");
}

// -------------------------------
// MAIN
// -------------------------------
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("[X] Could not connect.\n");
        return 1;
    }

    printf("[+] Connected to server.\n");

    // -------------------------------
    // ASK USERNAME
    // -------------------------------
    char username[50];
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    char login_packet[100];
    snprintf(login_packet, sizeof(login_packet), "LOGIN|%s", username);
    send(sock, login_packet, strlen(login_packet), 0);

    // -------------------------------
    // START RECEIVE THREAD
    // -------------------------------
    unsigned threadID;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, receive_messages, NULL, 0, &threadID);
    CloseHandle(hThread);

    // -------------------------------
    // MAIN LOOP (SEND CHAT/FILE)
    // -------------------------------
    char input[1024];

    while (1) {
        printf("> ");
        fgets(input, sizeof(input), stdin);

        if (strncmp(input, "/file ", 6) == 0) {
            input[strcspn(input, "\n")] = 0;
            send_file(input + 6);
            continue;
        }

        send(sock, input, strlen(input), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
