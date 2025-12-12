#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096
#define SERVER_PORT 4444

SOCKET server_socket;

DWORD WINAPI recv_thread(LPVOID arg){
    (void)arg;
    while(1){
        uint32_t netlen; int r=recv(server_socket,(char*)&netlen,sizeof(netlen),MSG_WAITALL);
        if(r<=0) break;
        uint32_t len=ntohl(netlen);
        char *buf=malloc(len+1); if(!buf) break;
        uint32_t total=0;
        while(total<len){
            int got=recv(server_socket,buf+total,len-total,0);
            if(got<=0) break;
            total+=got;
        }
        buf[len]=0; printf("%s\n",buf); free(buf);
    }
    printf("Disconnected from server.\n");
    return 0;
}

int send_msg(const char *msg){
    uint32_t len=(uint32_t)strlen(msg); uint32_t netlen=htonl(len);
    if(send(server_socket,(char*)&netlen,sizeof(netlen),0)!=sizeof(netlen)) return -1;
    if(send(server_socket,msg,len,0)!=(int)len) return -1;
    return 0;
}

int main(){
    WSADATA wsa; if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){ printf("WSAStartup failed\n"); return 1;}
    server_socket=socket(AF_INET,SOCK_STREAM,0);
    if(server_socket==INVALID_SOCKET){ printf("Socket failed\n"); return 1;}

    char ip[64]; printf("Enter server IP: "); scanf("%63s",ip); getchar();
    struct sockaddr_in addr={0}; addr.sin_family=AF_INET; addr.sin_port=htons(SERVER_PORT);
    addr.sin_addr.s_addr=inet_addr(ip);

    if(connect(server_socket,(struct sockaddr*)&addr,sizeof(addr))!=0){ printf("Connect failed\n"); return 1;}

    char username[64]; printf("Enter username: "); fgets(username,sizeof(username),stdin);
    username[strcspn(username,"\r\n")]=0;
    char loginmsg[BUFFER_SIZE]; snprintf(loginmsg,sizeof(loginmsg),"LOGIN|%s",username); send_msg(loginmsg);

    HANDLE hRecv=CreateThread(NULL,0,recv_thread,NULL,0,NULL);

    char input[BUFFER_SIZE];
    while(1){
        if(!fgets(input,sizeof(input),stdin)) continue;
        input[strcspn(input,"\r\n")]=0; if(strcmp(input,"")==0) continue;

        // /join [channel] prompt
        if(strncmp(input,"/join ",6)==0){
            char channel[64]; strcpy(channel,input+6);
            printf("Do you want to create private channel? (y/n): "); char ans=getchar(); getchar();
            if(ans=='y'){
                char psw[64]; printf("Enter password: "); fgets(psw,sizeof(psw),stdin); psw[strcspn(psw,"\r\n")]=0;
                char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"JOIN|%s|%s",channel,psw); send_msg(msg); continue;
            } else {
                char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"JOIN|%s|",channel); send_msg(msg); continue;
            }
        }

        // /pm
        if(strncmp(input,"/pm ",4)==0){
            char *space=strchr(input+4,' '); if(!space){ printf("Usage: /pm <user> <msg>\n"); continue;}
            *space=0; char *target=input+4; char *msgptr=space+1;
            char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"PM|%s|%s",target,msgptr); send_msg(msg); continue;
        }

        // /nick
        if(strncmp(input,"/nick ",6)==0){
            char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"NICK|%s",input+6); send_msg(msg); continue;
        }

        // /channels
        if(strcmp(input,"/channels")==0){
            char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"CHANNELS|"); send_msg(msg); continue;
        }

        // /users
        if(strncmp(input,"/users ",7)==0){
            char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"USERS|%s",input+7); send_msg(msg); continue;
        }

        if(strcmp(input,"/quit")==0) break;

        // normal message
        char msg[BUFFER_SIZE]; snprintf(msg,sizeof(msg),"MSG|%s",input); send_msg(msg);
    }

    closesocket(server_socket); WSACleanup(); return 0;
}
