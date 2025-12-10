#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <pthread.h>

void *receive_messages(void *socket) {
    SOCKET sock = *(SOCKET*)socket;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);

        if (bytes > 0) {
            printf("\nMessage: %s\n", buffer);
        }
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(4444);
    server.sin_addr.s_addr = inet_addr("192.168.1.71");


    connect(sock, (struct sockaddr*)&server, sizeof(server));

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &sock);
    pthread_detach(thread);

    char msg[1024];
    while (1) {
        fgets(msg, 1024, stdin);
        send(sock, msg, strlen(msg), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
