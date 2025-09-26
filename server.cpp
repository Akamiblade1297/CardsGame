#include <algorithm>
#include <asm-generic/socket.h>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <ostream>
#include <string>
#include <cstring>
#include <thread>
#include <unistd.h>
#include "main.h"

#define PORT 8494
#define CONN_REQ_LEN 25

Table table;
WorkerQueue queue;
PlayerManager playerMgr(&queue);
struct sockaddr_in address;
int addrlen = sizeof(address);
int server;
int reuse = 1;

std::vector<std::string> split( char* str, char del ) {
    std::vector<std::string> str_vec;
    std::stringstream str_stream(str);
    std::string s;
    
    while ( std::getline(str_stream, s, del) ) {
        str_vec.push_back(s);
    }

    return str_vec;
}

void log_msg ( std::string log ) {
    std::cout << log << std::endl;
}

void worker () {
    while ( true ) {
        std::string task = queue.pop();
        log_msg("Task: "+task);
        
        // Processing task(eg. "SAY\nHi!!", "WSP\nPlayer2\nPss, hey")
    }
}

void establisher () {
    std::string name;
    std::string address_string;
    char del = '\n';
    char buffer[CONN_REQ_LEN] = {0};
    struct sockaddr_in client_address;
    uint32_t pass;
    int client;
    while ( true ) {
        client = accept(server, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        inet_ntop(AF_INET, &(client_address.sin_addr), buffer, INET_ADDRSTRLEN);
        address_string = buffer;
        log_msg(address_string+" connected to server");

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        read(client, buffer, CONN_REQ_LEN);

        std::vector<std::string> request = split(buffer, del);

        if ( request.size() == 3 && request[0] == "JOIN") {
            std::memcpy(&pass, request[1].data(), 4);
            name = request[2];
            switch ( playerMgr.join(pass, name, client) ) {
                case 0:
                    log_msg(address_string+" assigned as "+name);
                    strcpy(buffer, "OK");
                    send(client, buffer, 2, 0);
                    break;
                case 1:
                    Player* player = playerMgr.playerByPass(pass);
                    log_msg(address_string+" assigned as "+player->Name);
                    strcpy(buffer, "RENAMED");
                    send(client, buffer, 7, 0);
                    break;
            }
        } else if ( request.size() == 2 && request[0] == "REJOIN" ) { 
            std::memcpy(&pass, request[1].data(), 4);
            log_msg(std::to_string(pass));
            Player* player = playerMgr.playerByPass(pass);
            if ( player == nullptr ) {
                log_msg(address_string+" failed to rejoin");
                strcpy(buffer, "NOT FOUND");
                send(client, buffer, 9, 0);
                close(client);
            } else {
                player->ConnectionSocket = client;
                log_msg(address_string+" rejoined as "+player->Name);
                strcpy(buffer, "OK");
                send(client, buffer, 2, 0);
            }
        }
        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
    }
}

int main () {
    server = socket(AF_INET, SOCK_STREAM, 0);
    if ( server == 0 ) {
        log_msg("ERROR: Failed to create server socket");
        exit(1);
    }
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    address.sin_family      = AF_INET;
    address.sin_port        = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if ( bind(server, (struct sockaddr*)&address, addrlen) < 0 ) {
        log_msg("ERROR: Failed to bind server socket");
        exit(1);
    }
    listen(server, 5);

    std::thread worker_thread(worker);
    std::thread establisher_thread(establisher);

    worker_thread.detach();
    establisher_thread.join();

    return 0;
}
