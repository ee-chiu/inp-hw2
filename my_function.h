#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int Socket(int family, int type, int protocol){
    int sockfd = socket(family, type, protocol);
    if(sockfd < 0){
        perror("Socket error");
        exit(0);
    }

    return sockfd;
} 

int Bind(int listenfd, struct sockaddr * ptr_srv_addr, int size){
    int b = bind(listenfd, ptr_srv_addr, size);

    if(b < 0){
        perror("Bind error");
        exit(0);
    }

    return 0;
}

int Listen(int listenfd, int backlog){
    int l = listen(listenfd, backlog);
    
    if(l < 0){
        perror("Listen error");
        exit(0);
    }

    return 0;
}

int Accept(int listenfd){
    int a = accept(listenfd, (struct sockaddr *) NULL, NULL);

    if (a < 0){
        perror("Accept error");
        exit(0);
    }

    return a;
}

int Close(int socket){
    int c = close(socket);
    if (c < 0){
        perror("Close error");
        exit(0);
    }

    return 0;
}

int Read(int socket, void* buffer, unsigned int size){
    int r = read(socket, buffer, size);
    if(r < 0){
        perror("Read error");
        exit(0);
    }

    return r;
}

ssize_t Write(int connfd, const char * cli_buff, size_t nbyte){
    ssize_t w = write(connfd, cli_buff, strlen(cli_buff));
    if (w < 0){
        perror("Write error");
        exit(0);
    }

    return w;
}

int Shutdown(int connfd, int how){
    int s = shutdown(connfd, how);
    if(s < 0){
        perror("Shutdown error");
        exit(0);
    }

    return s;
}


