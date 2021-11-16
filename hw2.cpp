#include "my_function.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/select.h>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <iostream>
#include <sstream>
#include <ctime>
#include <algorithm>

using namespace std;

struct post{   
    int id;
    string title;
    string content;
    string board;
    string author;
    string date;
};

char cli_buff[10000];
char srv_buff[10000];
const int max_size = 20;
vector<bool> isLogin(max_size, false);
vector<string> user(max_size, "");
map<string, string> user2password;
map< string, map< string, queue<string> > > user2other_user_message;
vector<string> board_list;
map<string, string> board2moderator;
map<int, post> num2post;
int number = 1;

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

string get_single_para(string command, int &i){
    string para;
    while(command[i] != 0 && command[i] != ' '){
        para += command[i];
        i++;
    }    

    while(command[i] == ' ')
        i++;
    
    return para;
}

string get_space_para(string command, int &i){
    string para;
    while(command[i] != 0 && command[i] != '-'){
        para += command[i];
        i++;
    }

    while(para.size() > 0 && para.back() == ' '){
        para.pop_back();
    }

    return para;
}

void get_create_post_para(string command, int &i, vector<string> &para){
    string board_name = get_single_para(command, i);
    para.push_back(board_name);

    for(int j = 1 ; j <= 2 ; j++){
        string subcommand = get_single_para(command, i);
        if(subcommand.size() > 0)
            para.push_back(subcommand);

        string content = get_space_para(command, i);
        if(content.size() > 0)
            para.push_back(content);
    }        

    return;
}

void get_other_para(string command, int &i, vector<string> &para){
    string tmp;
    while(command[i] != 0){
        tmp = get_single_para(command, i);
        para.push_back(tmp);
    }

    return;
}

vector<string> split(string command){
    vector<string> para;
    int i = 0;
    string first_para = get_single_para(command, i);
    para.push_back(first_para);

    if(first_para == "create-post"){
        get_create_post_para(command, i, para);
        return para;
    }

    get_other_para(command, i, para);
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

void create_board(int sockfd, const vector<string> &para){
    if(para.size() != 2){
        write2cli(sockfd, "Usage: create-board <name>\n");
        return;
    }

    if(!isLogin[sockfd]){
        write2cli(sockfd, "Please login first.\n");
        return;
    }

    if(std::find(board_list.begin(), board_list.end(), para[1]) != board_list.end()){
        write2cli(sockfd, "Board already exists.\n");
        return;
    }

    board_list.push_back(para[1]);
    board2moderator[para[1]] = user[sockfd];
    write2cli(sockfd, "Create board successfully.\n");
    return;
}

void list_board(int sockfd){
    write2cli(sockfd, "Index Name Moderator\n");
    for(int i = 0 ; i < board_list.size() ; i++){
        snprintf(cli_buff, sizeof(cli_buff), 
                 "%d %s %s\n", 
                 i + 1, board_list[i].c_str(), board2moderator[board_list[i]].c_str());   
        Write(sockfd, cli_buff, strlen(cli_buff));
    }
    
    return;
}

string get_title(const vector<string> &para){
    for(int i = 0 ; i + 1 < para.size() ; i++){
        if(para[i] == "--title")
            return para[i + 1];
    }

    return "";
}

bool is_br(int i, string content){
    string br;
    for(int j = 1 ; j <= 4 ; j++){
        br += content[i];
        i++;
    }

    if(br == "<br>")
        return true;
    
    return false;
}

string get_content(const vector<string> &para){
    string content;
    for(int i = 0 ; i + 1 < para.size() ; i++){
        if(para[i] == "--content"){
            content = para[i + 1];    
            break;
        }
    }

    int i = 0; 
    string tmp;
    while(i < content.size()){
        if(content[i] == '<' && is_br(i, content)) {
            tmp += '\n';
            i += 4;
            continue;
        }

        tmp += content[i];
        i++;
    }

    return tmp;
}

string get_date(){
    time_t now = time(0);
    tm *local_time = localtime(&now);
    int month = 1 + local_time->tm_mon;
    int day = local_time->tm_mday;

    stringstream ss;

    string month_s;
    string day_s;
    string date;

    ss << month;
    ss >> month_s;

    ss.clear();

    ss << day;
    ss >> day_s;
    
    date = month_s + "/" + day_s;

    return date;
}

void create_post(int sockfd, const vector<string> &para){
    if(para.size() != 6){
        write2cli(sockfd, "Usage: create-post <board-name> --title <title> --content <content>\n");
        return;
    }

    if(!isLogin[sockfd]){
        write2cli(sockfd, "Please login first.\n");
        return;
    }

    if(std::find(board_list.begin(), board_list.end(), para[1]) == board_list.end()){
        write2cli(sockfd, "Board does not exist.\n");
        return;
    }

    post p;
    p.id = number;
    p.title = get_title(para);
    p.content = get_content(para);
    p.board = para[1];
    p.author = user[sockfd];
    p.date = get_date();
    
    num2post[number] = p;
    
    number++;
    //post_list.push_back(p);

    write2cli(sockfd, "Create post successfully.\n");
    return;
}

void list_post(int sockfd, const vector<string> &para){
    if(para.size() != 2){
        write2cli(sockfd, "Usage: list-post <board-name>\n");
        return;
    }

    if(std::find(board_list.begin(), board_list.end(), para[1]) == board_list.end()){
        write2cli(sockfd, "Board does not exist.\n");
        return;
    }

    write2cli(sockfd, "S/N Title Author Date\n");
    for(map<int, post>::iterator it = num2post.begin() ; it != num2post.end() ; it++){
        post p = it -> second; 
        if(p.board == para[1]){
            snprintf(cli_buff, sizeof(cli_buff), 
                    "%d %s %s %s\n", 
                    it -> first, p.title.c_str(), p.author.c_str(), p.date.c_str());
            Write(sockfd, cli_buff, strlen(cli_buff));
        }
    }
    /*for(int i = 0 ; i < post_list.size() ; i++){
        if(post_list[i].board == para[1]){
            post p = post_list[i];
            snprintf(cli_buff, sizeof(cli_buff), 
                    "%d %s %s %s\n", 
                    p.id, p.title.c_str(), p.author.c_str(), p.date.c_str());
            Write(sockfd, cli_buff, strlen(cli_buff));
        }
    }*/

    return;
}

int str2int(string s){
    int i = 0;
    int num = 0;
    while(i < s.size()){
        num = num * 10 + s[i] - '0';
        i++;
    }

    return num;
}

void read_post(int sockfd, const vector<string> &para){
    if(para.size() != 2){
        write2cli(sockfd, "Usage: read <post-S/N>\n");
        return;
    }

    int id = str2int(para[1]);
    if(num2post.find(id) == num2post.end()){
        write2cli(sockfd, "Post does not exist.\n");
        return;
    }

    snprintf(cli_buff, sizeof(cli_buff), "Author: %s\n", num2post[id].author.c_str());
    Write(sockfd, cli_buff, strlen(cli_buff));

    snprintf(cli_buff, sizeof(cli_buff), "Title: %s\n", num2post[id].title.c_str());
    Write(sockfd, cli_buff, strlen(cli_buff));

    snprintf(cli_buff, sizeof(cli_buff), "Date: %s\n", num2post[id].date.c_str());
    Write(sockfd, cli_buff, strlen(cli_buff));

    write2cli(sockfd, "--\n");

    snprintf(cli_buff, sizeof(cli_buff), "%s\n", num2post[id].content.c_str());
    Write(sockfd, cli_buff, strlen(cli_buff));

    write2cli(sockfd, "--\n");

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
        else if(para[0] == "create-board") create_board(sockfd, para);
        else if(para[0] == "list-board") list_board(sockfd);
        else if(para[0] == "create-post") create_post(sockfd, para);
        else if(para[0] == "list-post") list_post(sockfd, para);
        else if(para[0] == "read") read_post(sockfd, para);
        
    }
    write2cli(sockfd, "% ");
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
            write2cli(connectfd, "% ");

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