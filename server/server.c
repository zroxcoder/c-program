#include "server.h"
#include "client_handler.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Client clients[MAX_CLIENTS];
int client_count = 0;

Channel channels[MAX_CHANNELS];
int channel_count = 1; // default "general"

FILE *log_file = NULL;

void log_event(const char *msg) {
    log_file = fopen("server_logs.txt","a");
    if(log_file){
        char ts[64]; make_timestamp(ts);
        fprintf(log_file,"[%s] %s\n",ts,msg);
        fclose(log_file);
    }
    printf("[LOG] %s\n",msg);
}

void print_active_clients() {
    printf("\n=== Active clients (%d) ===\n",client_count);
    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active){
            printf("Index=%d Name=%s Channel=%s Socket=%llu Muted=%d\n",
                i, clients[i].username, clients[i].channel,
                (unsigned long long)clients[i].socket, clients[i].muted);
        }
    }
    printf("===========================\n");
}

void list_channels(SOCKET sock) {
    char msg[BUFFER_SIZE]; msg[0]=0;
    strcat(msg,"[SERVER] Channels:\n");
    for(int i=0;i<channel_count;i++){
        char tmp[128];
        snprintf(tmp,sizeof(tmp),"%s (%d users) %s\n",channels[i].name,channels[i].user_count,
            strlen(channels[i].password)? "[PRIVATE]":"[PUBLIC]");
        strcat(msg,tmp);
    }
    send_msg_sock(sock,msg,(uint32_t)strlen(msg));
}

void list_users(SOCKET sock, const char *chan) {
    char msg[BUFFER_SIZE]; msg[0]=0;
    strcat(msg,"[SERVER] Users:\n");
    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active && strcmp(clients[i].channel,chan)==0){
            strcat(msg,clients[i].username);
            strcat(msg,"\n");
        }
    }
    send_msg_sock(sock,msg,(uint32_t)strlen(msg));
}

// Admin console
DWORD WINAPI admin_console_thread(LPVOID arg){
    (void)arg;
    char cmd[BUFFER_SIZE];
    while(1){
        printf("server> "); fflush(stdout);
        if(!fgets(cmd,sizeof(cmd),stdin)){ Sleep(100); continue;}
        cmd[strcspn(cmd,"\r\n")]=0;
        if(strcmp(cmd,"")==0) continue;

        if(strncmp(cmd,"list",4)==0){ print_active_clients(); continue;}
        if(strncmp(cmd,"kick ",5)==0){
            char name[64]; sscanf(cmd+5,"%63s",name);
            for(int i=0;i<MAX_CLIENTS;i++){
                if(clients[i].active && strcmp(clients[i].username,name)==0){
                    closesocket(clients[i].socket);
                    clients[i].active=0;
                    char logmsg[128]; snprintf(logmsg,sizeof(logmsg),"Admin kicked: %s",name);
                    log_event(logmsg); break;
                }
            } continue;
        }
        if(strncmp(cmd,"mute ",5)==0){
            char name[64]; sscanf(cmd+5,"%63s",name);
            for(int i=0;i<MAX_CLIENTS;i++){
                if(clients[i].active && strcmp(clients[i].username,name)==0){
                    clients[i].muted=1;
                    char logmsg[128]; snprintf(logmsg,sizeof(logmsg),"Admin muted: %s",name);
                    log_event(logmsg); break;
                }
            } continue;
        }
        if(strncmp(cmd,"unmute ",7)==0){
            char name[64]; sscanf(cmd+7,"%63s",name);
            for(int i=0;i<MAX_CLIENTS;i++){
                if(clients[i].active && strcmp(clients[i].username,name)==0){
                    clients[i].muted=0;
                    char logmsg[128]; snprintf(logmsg,sizeof(logmsg),"Admin unmuted: %s",name);
                    log_event(logmsg); break;
                }
            } continue;
        }
        if(strncmp(cmd,"broadcast ",10)==0){
            char *msgptr = cmd+10;
            char *space = strchr(msgptr,' ');
            char out[BUFFER_SIZE];
            char ts[64]; make_timestamp(ts);
            if(space){
                char channel[64]; *space=0; strcpy(channel,msgptr); msgptr=space+1;
                snprintf(out,sizeof(out),"[%s] [SERVER] %s",ts,msgptr);
                broadcast_channel(out,channel,-1);
            }else{
                snprintf(out,sizeof(out),"[%s] [SERVER] %s",ts,msgptr);
                for(int i=0;i<MAX_CLIENTS;i++){
                    if(clients[i].active) send_msg_sock(clients[i].socket,out,(uint32_t)strlen(out));
                }
            }
            char logmsg[BUFFER_SIZE]; snprintf(logmsg,sizeof(logmsg),"Admin broadcast: %s",msgptr);
            log_event(logmsg); continue;
        }
        if(strcmp(cmd,"help")==0){
            printf("Commands: list, kick <user>, mute <user>, unmute <user>, broadcast [channel] <msg>\n");
            continue;
        }

        printf("Unknown command. Use 'help'\n");
    }
    return 0;
}

int main(){
    WSADATA wsa; if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){ printf("WSAStartup failed\n"); return 1;}
    SOCKET server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket==INVALID_SOCKET){ printf("Socket failed\n"); return 1;}

    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET; addr.sin_port=htons(SERVER_PORT);
    addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_socket,(struct sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){ printf("Bind failed\n"); return 1;}
    if(listen(server_socket,10)==SOCKET_ERROR){ printf("Listen failed\n"); return 1;}

    log_event("SERVER STARTED");

    for(int i=0;i<MAX_CLIENTS;i++){
        clients[i].active=0; clients[i].muted=0;
        strcpy(clients[i].channel,"general");
        strcpy(clients[i].username,"Unknown");
    }
    strcpy(channels[0].name,"general");
    channels[0].user_count=0; channels[0].history_count=0;
    channels[0].password[0]=0;

    HANDLE hAdmin=CreateThread(NULL,0,admin_console_thread,NULL,0,NULL);
    CloseHandle(hAdmin);

    while(1){
        SOCKET client_sock=accept(server_socket,NULL,NULL);
        if(client_sock==INVALID_SOCKET) continue;

        int i; for(i=0;i<MAX_CLIENTS;i++){
            if(!clients[i].active){ clients[i].socket=client_sock; clients[i].active=1; break;}
        }
        if(i==MAX_CLIENTS){ closesocket(client_sock); continue;}

        int *pidx=malloc(sizeof(int)); *pidx=i;
        HANDLE h=CreateThread(NULL,0,client_thread_wrapper,pidx,0,&clients[i].threadId);
        CloseHandle(h);
    }

    closesocket(server_socket); WSACleanup(); return 0;
}
