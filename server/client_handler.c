#include "client_handler.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// Thread wrapper for a client
DWORD WINAPI client_thread_wrapper(LPVOID arg) {
    int index = *(int*)arg;
    free(arg);

    // Optional: increment/decrement global client count safely
    InterlockedIncrement((volatile LONG*)&client_count);

    // Handle this client
    handle_client(index);

    InterlockedDecrement((volatile LONG*)&client_count);
    return 0;
}

extern Client clients[];
extern int client_count;
extern Channel channels[];
extern int channel_count;
extern FILE *log_file;

void make_timestamp(char *out) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(out, "%04d-%02d-%02d %02d:%02d:%02d",
        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec);
}

int send_msg_sock(SOCKET s, const void *buf, uint32_t len) {
    uint32_t netlen = htonl(len);
    if (send(s, (const char*)&netlen, sizeof(netlen), 0) != sizeof(netlen)) return -1;
    uint32_t total = 0;
    const char *p = buf;
    while (total < len) {
        int sent = send(s, p + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return 0;
}

int recv_msg_sock(SOCKET s, char **out, uint32_t *out_len) {
    uint32_t netlen;
    int r = recv(s, (char*)&netlen, sizeof(netlen), MSG_WAITALL);
    if (r <= 0) return -1;
    uint32_t len = ntohl(netlen);
    char *buf = malloc(len + 1);
    if (!buf) return -1;
    uint32_t total = 0;
    while (total < len) {
        int got = recv(s, buf + total, len - total, 0);
        if (got <= 0) { free(buf); return -1; }
        total += got;
    }
    buf[len] = '\0';
    *out = buf;
    *out_len = len;
    return 0;
}

void broadcast_channel(const char *msg, const char *channel, SOCKET exclude) {
    for (int i=0;i<MAX_CLIENTS;i++) {
        if (clients[i].active && strcmp(clients[i].channel, channel) == 0 && clients[i].socket != exclude && !clients[i].muted) {
            send_msg_sock(clients[i].socket, msg, (uint32_t)strlen(msg));
        }
    }
}

void send_private(int index, const char *target, const char *msg) {
    for (int i=0;i<MAX_CLIENTS;i++) {
        if (clients[i].active && strcmp(clients[i].username, target) == 0) {
            char buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "[PM from %s] %s", clients[index].username, msg);
            send_msg_sock(clients[i].socket, buffer, (uint32_t)strlen(buffer));
            char logmsg[BUFFER_SIZE];
            snprintf(logmsg, sizeof(logmsg), "PM from %s to %s: %s", clients[index].username, target, msg);
            log_event(logmsg);
            return;
        }
    }
    char notice[200];
    snprintf(notice, sizeof(notice), "[SERVER] User '%s' not found.", target);
    send_msg_sock(clients[index].socket, notice, (uint32_t)strlen(notice));
}

void handle_client(int index) {
    SOCKET sock = clients[index].socket;
    char *msg = NULL;
    uint32_t msglen = 0;

    while(1) {
        if (recv_msg_sock(sock, &msg, &msglen) != 0) {
            clients[index].active = 0;
            closesocket(sock);
            char logmsg[128];
            snprintf(logmsg, sizeof(logmsg), "Client disconnected: index=%d user=%s", index, clients[index].username);
            log_event(logmsg);
            return;
        }

        // LOGIN
        if (msglen >=6 && strncmp(msg,"LOGIN|",6)==0) {
            strncpy(clients[index].username, msg+6, sizeof(clients[index].username)-1);
            clients[index].username[strcspn(clients[index].username,"\r\n")]=0;

            char announce[BUFFER_SIZE];
            snprintf(announce,sizeof(announce),"[SERVER] %s joined channel %s", clients[index].username, clients[index].channel);
            broadcast_channel(announce, clients[index].channel, sock);

            char logmsg[BUFFER_SIZE];
            snprintf(logmsg,sizeof(logmsg),"LOGIN: index=%d name=%s",index,clients[index].username);
            log_event(logmsg);

            free(msg); continue;
        }

        // JOIN
        if (msglen>=5 && strncmp(msg,"JOIN|",5)==0) {
            char newchan[64];
            strncpy(newchan,msg+5,sizeof(newchan)-1);
            newchan[strcspn(newchan,"\r\n")]=0;

            char oldchan[64]; strcpy(oldchan, clients[index].channel);
            strcpy(clients[index].channel,newchan);

            char notify[BUFFER_SIZE];
            snprintf(notify,sizeof(notify),"[SERVER] %s moved from %s to %s", clients[index].username, oldchan,newchan);
            broadcast_channel(notify, oldchan, -1);
            broadcast_channel(notify, newchan, -1);
            log_event(notify);

            free(msg); continue;
        }

        // PM
        if (msglen>=3 && strncmp(msg,"PM|",3)==0) {
            char *copy = _strdup(msg);
            char *tok = strtok(copy,"|"); tok = strtok(NULL,"|");
            char target[64]; if(tok) strncpy(target,tok,sizeof(target)-1); else strcpy(target,"");
            tok = strtok(NULL,"\0"); char *pm = tok?tok:"";
            send_private(index,target,pm);
            free(copy); free(msg); continue;
        }

        // MSG
        if (msglen>=4 && strncmp(msg,"MSG|",4)==0) {
            char ts[64]; make_timestamp(ts);
            char out[BUFFER_SIZE];
            snprintf(out,sizeof(out),"[%s] %s: %s",ts,clients[index].username,msg+4);
            broadcast_channel(out,clients[index].channel,sock);

            char logmsg[BUFFER_SIZE];
            snprintf(logmsg,sizeof(logmsg),"MSG from %s: %s",clients[index].username,msg+4);
            log_event(logmsg);
            free(msg); continue;
        }

        free(msg);
    }
}
