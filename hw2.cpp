#include "my_function.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/select.h>

using namespace std;

char cli_buff[10000];
char srv_buff[10000];

int char2int(char* c){
    int i = 0;
    int num = 0;

    while(c[i] != 0){
        num = num * 10 + c[i] - '0';
        i++;
    }

    return num;
} 

void write2cli(int connfd,const char * message){
    snprintf(cli_buff, sizeof(cli_buff), "%s", message);
    Write(connfd, cli_buff, strlen(cli_buff));
    return;
}

void welcome(int connectfd){
    write2cli(connectfd, "********************************\n** Welcome to the BBS server. **\n********************************\n");
    return;
}

int main(int argc, char** argv){
    if(argc != 2){
        printf("Usage: ./hw2 <port>\n");
        return 0;
    }

    int listenfd, connectfd;
    struct sockaddr_in srv_addr;
    const int max_size = 20;
    
    int port = char2int(argv[1]);
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);    
    setsockopt(listenfd, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT), NULL, 0);

    Bind(listenfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    const int backlog = max_size;
    Listen(listenfd, backlog);

    fd_set all_set;
    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);

    int cli[max_size];
    int i = 0;
    int maxi = -1;
    int maxfd = listenfd;
    
    for(i = 0 ; i < max_size ; i++)
        cli[i] = -1;

    while(1){
        memset(cli_buff, 0, sizeof(cli_buff));
        memset(srv_buff, 0, sizeof(srv_buff));

        fd_set rset = all_set;
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset)){
            connectfd = Accept(listenfd);

            for(i = 0 ; i < max_size ; i++){
                if(cli[i] < 0) {
                    cli[i] = connectfd;
                    break;
                }
            }

            FD_SET(connectfd, &all_set);

            welcome(connectfd);

            if(connectfd > maxfd) maxfd = connectfd;
            if(i > maxi) maxi = i;
            if(--nready <= 0) continue;
        }

        for(i = 0 ; i <= maxi ; i++){
            if (cli[i] < 0) continue;
            int sockfd = cli[i];
            if(FD_ISSET(sockfd, &rset)){
                Read(sockfd, srv_buff, sizeof(srv_buff));
                printf("%s\n", srv_buff);
            }
        }
    }
}