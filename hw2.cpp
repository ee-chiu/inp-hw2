#include "my_function.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/select.h>
#include <vector>
#include <string>
#include <map>
#include <queue>

using namespace std;

char cli_buff[10000];
char srv_buff[10000];
const int max_size = 20;
vector<bool> isLogin(max_size, false);
vector<string> user(max_size, "");
map<string, string> user2password;
map< string, map< string, queue<string> > > user2other_user_message;

fd_set all_set;

int char2int(char* c){
    int i = 0;
    int num = 0;

    while(c[i] != 0){
        num = num * 10 + c[i] - '0';
        i++;
    }

    return num;
} 

vector<string> split(string command){
    vector<string> para;
    string tmp;
    int i = 0;
    while(command[i] != 0){
        if(command[i] == '"') {
            i++;
            break;
        }
        if(command[i] == ' '){
            para.push_back(tmp);
            tmp.clear();
            i++;
            while(command[i] == ' ')
                i++;
            continue;
        }
        tmp += command[i];
        i++;
    }

    while(command[i] != 0 && command[i] != '"'){
        tmp += command[i];
        i++;
    }
    
    if(!tmp.empty())
        para.push_back(tmp);
    return para;
}

void write2cli(int sockfd,const char * message){
    snprintf(cli_buff, sizeof(cli_buff), "%s", message);
    Write(sockfd, cli_buff, strlen(cli_buff));
    return;
}

void write2cli2(int sockfd, const char * text, const char * user){
    snprintf(cli_buff, sizeof(cli_buff), "%s, %s.\n", text, user);
    Write(sockfd, cli_buff, strlen(cli_buff));
    return;
}

void welcome(int sockfd){
    write2cli(sockfd, "********************************\n** Welcome to the BBS server. **\n********************************\n");
    return;
}

void Exit(int sockfd){
    if(isLogin[sockfd]) {
        snprintf(cli_buff, sizeof(cli_buff), "Bye, %s.", user[sockfd].c_str());
        Write(sockfd, cli_buff, strlen(cli_buff));
    }
    isLogin[sockfd] = false;
    user[sockfd].clear();                

    Close(sockfd);

    FD_CLR(sockfd, &all_set);

    return;
}

void reg(int sockfd, const vector<string> &para){
    if(para.size() != 3){
        write2cli(sockfd, "Usage: register <username> <password>\n");
        return;
    }

    auto itr = user2password.find(para[1]);
    if(itr != user2password.end()){
        write2cli(sockfd, "Username is already used.\n");
        return;
    }

    write2cli(sockfd, "Register successfully.\n");
    user2password[para[1]] = para[2];
    return;
}

void login(int sockfd,const vector<string> &para){
    if(para.size() != 3){
        write2cli(sockfd, "Usage: login <username> <password>\n");
        return;
    }

    if(isLogin[sockfd]){
        write2cli(sockfd, "Please logout first.\n");
        return;
    }

    auto itr = user2password.find(para[1]);
    if(itr == user2password.end()){
        write2cli(sockfd, "Login failed.\n");
        return;
    }

    if(user2password[para[1]] != para[2]){
        write2cli(sockfd, "Login failed.\n");
        return;
    }

    write2cli2(sockfd, "Welcome", para[1].c_str());
    isLogin[sockfd] = true;
    user[sockfd] = para[1];
}

void logout(int sockfd){
    if(!isLogin[sockfd]){
        write2cli(sockfd, "Please login first.\n");
        return;
    }
    isLogin[sockfd] = false;
    write2cli2(sockfd, "Bye", user[sockfd].c_str());
    user[sockfd].clear();
    return;
}

void bbs_main(int sockfd){
    Read(sockfd, srv_buff, sizeof(srv_buff));
    if(srv_buff[0] != 0){
        string command(srv_buff);
        command.pop_back();
        vector<string> para = split(command);
        if(para[0] == "exit"){
            Exit(sockfd);
            return;
        }

        if(para[0] == "register") reg(sockfd, para);
        else if(para[0] == "login") login(sockfd, para);
        else if(para[0] == "logout") logout(sockfd);
    }
}

int main(int argc, char** argv){
    if(argc != 2){
        printf("Usage: ./hw2 <port>\n");
        return 0;
    }

    int listenfd, connectfd;
    struct sockaddr_in srv_addr;
    
    int port = char2int(argv[1]);
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);    

    Bind(listenfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    const int backlog = max_size;
    Listen(listenfd, backlog);

    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);

    vector<int> cli(max_size, -1);
    int i = 0;
    int maxi = -1;
    int maxfd = listenfd;
    
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
                bbs_main(sockfd);
            }
        }
    }
}