#include "main.h"
#include "functions.cpp"
#include "string_func.cpp"
#include <asm-generic/socket.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <csignal>
#include <thread>

#define NOTIFY temp+DEL+player->Name+DEL+req
#define UNK ( card->Trap ? "TRAP" : "TRES" )

// void verb ( std::string msg ) {
//     if ( verbose ) std::cout << msg << std::endl;
// }

Log logger("logs/log");
// bool verbose = false;
WorkerQueue queue;
PlayerManager playerMgr(&queue, &logger, rd);
struct sockaddr_in address;
int addrlen = sizeof(address);
int server;
int reuse = 1;

void worker () {
    char buffer[BUF];
    while ( true ) {
        std::fill(buffer, buffer+BUF, 0);
        auto [player, req] = queue.pop();
        strcpy(buffer, req.c_str());
        std::vector<std::string> request = split(buffer, DEL);
        
        if ( request.size() == 2 && ( request[0] == "CHAT" || request[0] == "ACT" ) ) { // CHAT <message>
            playerMgr.sendAll(request[0] + DEL + player->Name + DEL + request[1]);      // ACT  <action>
            std::vector<std::string> temp;
            if ( request[0] == "CHAT" ) { temp.push_back(": ");temp.push_back("") ; }
            else                        { temp.push_back(" *");temp.push_back("*"); }

            logger.log(player->Name + temp[0] + request[1] + temp[1]);
        } else if ( request.size() == 3 && request[0] == "WHISPER" ) { // WHISPER <player> <message>
            Player* player_dest = playerMgr.playerByName(request[1]);
            std::string temp = "WHISPER";
            if ( playerMgr.sendByName(request[1], temp + DEL + player->Name + DEL + request[2]) == 0 ) {
                player->sendMsg("OK");
                logger.log(player->Name+" -> "+request[1]+": "+request[2]);
            } else {
                player->sendMsg("NOT FOUND");
            }
        } else if ( request.size() == 5 && request[0] == "DECK" ) { // DECK <deck src> <dest> <x> <y>
            Deck* src           = deckByName(request[1]);
            CardContainer* dest = containerByName(request[2], player);
            Card* card = nullptr;
            if ( src == dest || src == nullptr || dest == nullptr ) {
                player->sendMsg("NOT FOUND");
                continue;
            }
            card = src->pop_and_move(dest);
            if ( card == nullptr ) {
                player->sendMsg("EMPTY");
            } else if ( transformCard(player, card, request[3], request[4]) != 0 ) {
                player->sendMsg("NOT FOUND");
            } else {
                logger.log(player->Name+" moved Card_"+std::to_string(card->Number)+" from "+request[1]+" to "+request[2]+" (X: "+request[3]+", Y: "+request[4]+")");
                std::string temp = "NOTIFY";
                playerMgr.sendAll(NOTIFY+DEL+( isVisible(request[2]) && card->Face ? std::to_string(card->Number) : UNK ));
            }
        } else if ( ( request.size() == 6 ) && request[0] == "MOVE" ) { // MOVE <spatial src> <card_id> <dest> <x> <y>
            CardContainer* src  = spatialByName(request[1], player);
            CardContainer* dest = containerByName(request[3], player);
            Card* card;

            int i;
            try {
                i = std::stoi(request[2]);
            } catch ( std::out_of_range ) {
                player->sendMsg("OUT OF RANGE");
                continue;
            } catch ( std::invalid_argument ) {
                player->sendMsg("NOT A NUMBER");
                continue;
            }

            if ( src == nullptr || dest == nullptr ) {
                player->sendMsg("NOT FOUND");
                continue;
            }
            else if ( src == dest ) {
                if ( transformContainer(player, src, request[2], request[4], request[5]) != 0 ) {
                    continue;
                } else {
                    card = src->card(i);
                }
            } else {
                card = src->move(i, dest);
                if ( card == nullptr ) {
                    player->sendMsg("OUT OF RANGE");
                    continue;
                } else {
                    transformCard(player, card, request[4], request[5]);
                }
            }
            logger.log(player->Name+" moved Card_"+std::to_string(card->Number)+" from "+request[1]+" to "+request[3]+" (X: "+request[4]+", Y: "+request[5]+")");
            std::string temp = "NOTIFY";
            playerMgr.sendAll(NOTIFY+DEL+( isVisible(request[3]) && card->Face ? std::to_string(card->Number) : UNK ));
        } else if ( request.size() == 4 && request[0] == "ROTATE" ) { // ROTATE <spatial> <card> <rot>
            CardContainer* container = spatialByName(request[1], player);
            Card* card;

            int i;
            try {
                i = std::stoi(request[2]);
            } catch ( std::out_of_range ) {
                player->sendMsg("OUT OF RANGE");
                continue;
            } catch ( std::invalid_argument ) {
                player->sendMsg("NOT A NUMBER");
                continue;
            }

            if ( container == nullptr ) {
                player->sendMsg("NOT FOUND");
                continue;
            }

            if ( rotateContainer(player, container, request[2], request[3]) == 0 ) {
                card = container->card(i);
                logger.log(player->Name+" Rotated Card_"+std::to_string(card->Number)+" on "+request[1]+" (Rot: "+request[3]+")");
                std::string temp = "NOTIFY";
                playerMgr.sendAll(NOTIFY);
            }
        } else if ( request.size() == 3 && request[0] == "FLIP" ) { // FLIP <spatial> <card>
            CardContainer* container = spatialByName(request[1], player);

            if ( container == nullptr ) {
                player->sendMsg("NOT FOUND");
                continue;
            }

            int i;
            try {
                i = std::stoi(request[2]);
            } catch ( std::out_of_range ) {
                player->sendMsg("OUT OF RANGE");
                continue;
            } catch ( std::invalid_argument ) {
                player->sendMsg("NOT A NUMBER");
                continue;
            }

            Card* card = container->card(i);
            if ( card == nullptr ) {
                player->sendMsg("OUT OF RANGE");
            } else {
                card->flip();
                logger.log(player->Name+" flipped Card_"+std::to_string(card->Number)+" on "+request[1]);
                if ( isVisible(request[1]) ) {
                    std::string temp = "NOTIFY";
                    playerMgr.sendAll(NOTIFY+DEL+( isVisible(request[1]) ? std::to_string(card->Number) : UNK ));
                }
            }
        } else if ( request.size() == 2 && request[0] == "SHUFFLE" ) { // SHUFFLE <deck>
            Deck* deck = deckByName(request[1]);
            if ( deck == nullptr ) {
                player->sendMsg("NOT FOUND");
            } else {
                deck->shuffle();

                std::string temp = "NOTIFY";
                playerMgr.sendAll(NOTIFY);
            }
        } else if ( request.size() == 3 && request[0] == "SET" ) { // SET <stat> <value>
            std::string* sStat = sStatByName(request[1], player);
            int* iStat = iStatByName(request[1], player);
            if ( player == nullptr ) {
                player->sendMsg("NOT FOUND");
                continue;
            }
            if ( sStat == nullptr ) {
                if ( iStat == nullptr ) {
                    player->sendMsg("NOT FOUND");
                } else {
                    int i;
                    try {
                        i = std::stoi(request[2]);
                    } catch ( std::invalid_argument ) {
                        player->sendMsg("NOT A NUMBER");
                        continue;
                    } catch ( std::out_of_range ) {
                        player->sendMsg("OUT OF RANGE");
                        continue;
                    }
                    *iStat = i;
                }
            } else {
                *sStat = request[2];
            }
            logger.log(player->Name+"'s "+request[1]+" set to "+request[2]);
            std::string temp = "NOTIFY";
            playerMgr.sendAll(NOTIFY);
        } else if ( request.size() == 2 && request[0] == "RENAME") { // RENAME <new_name>
            logger.log(player->Name+" renamed to "+request[1]);
            std::string temp = "NOTIFY";
            playerMgr.sendAll(NOTIFY);
            player->Name = request[1];
        } else if (  request.size() == 2 && request[0] == "SEE" ) { // SEE <spatial/player>
            CardContainer* container = spatialByName(request[1], player);
            if ( container == nullptr ) {
                Player* plr = playerMgr.playerByName(request[1]);
                if ( plr == nullptr ) {
                    player->sendMsg("NOT FOUND");
                    continue;
                } else {
                    container = &plr->Equiped;
                }
            }
            std::vector<std::string> cards;
            for ( Card* card : container->Cards ) {
                std::string id;
                if ( card->Face ) {
                    id = std::to_string(card->Number);
                } else if ( card->Trap ) {
                    id = "TRAP";
                } else {
                    id = "TRES";
                }
                cards.push_back(id+' '+std::to_string(card->X)+' '+std::to_string(card->Y)+' '+std::to_string(card->Rotation));
            }
            std::string temp = "OK";
            player->sendMsg(temp + join(cards, std::string(1,DEL)) + DEL + "END");
        } else if ( request.size() == 2 && request[0] == "CARDS" ) { // CARDS <deck/player>
            CardContainer* container = deckByName(request[1]);
            if ( container == nullptr ) {
                Player* plr = playerMgr.playerByName(request[1]);
                if ( plr == nullptr ) {
                    player->sendMsg("NOT FOUND");
                    continue;
                } else {
                    container = &plr->Inventory;
                }
            }
            std::vector<std::string> cards;
            for ( Card* card : container->Cards ) {
                std::string id;
                if ( container == &player->Inventory ) {
                    id = std::to_string(card->Number);
                } else {
                    if ( card->Trap ) {
                        id = "TRAP";
                    } else {
                        id = "TRES";
                    }
                }
                cards.push_back(id+' '+std::to_string(card->X)+' '+std::to_string(card->Y)+' '+std::to_string(card->Rotation));
            }
            std::string temp = "OK";
            player->sendMsg(temp + join(cards, std::string(1,DEL)) + DEL + "END");
        } else if ( request.size() == 2 && request[0] == "STAT" ) { // STAT <player>
            Player* plr = playerMgr.playerByName(request[1]);
            if ( plr == nullptr ) {
                player->sendMsg("NOT FOUND");
            } else {
                std::string temp = "OK";
                player->sendMsg(temp+DEL+"LEVEL "+std::to_string(plr->Level)+DEL+"POWER "+std::to_string(plr->Power)+DEL+"GOLD "+std::to_string(plr->Gold));
            } 
        } else if ( request.size() == 1 && request[0] == "PLAYERS" ) { // PLAYERS
            std::vector<std::string> players;
            for ( int i = 1 ; i <= playerMgr.size() ; i++ ) players.push_back(playerMgr.playerById(i)->Name);
            std::string temp = "OK";
            player->sendMsg(temp+join(players, std::string(1,DEL))+DEL+"END");
        } else if ( request.size() == 2 && request[0] == "PING" && request[1].size() == 4 ) { // PING <random data>
            std::string temp = "PONG";
            player->sendMsg(temp + DEL + request[1]);
        } else {
            player->sendMsg("BAD REQUEST");
        }
    }
}

void establisher () {
    std::string name;
    std::string address_string;
    char buffer[CONN_REQ_LEN] = {0};
    struct sockaddr_in client_address;
    struct timeval timeout;
    fd_set set;
    uint64_t pass;
    int client;
    size_t len;

    while ( true ) {
        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        timeout.tv_sec = 5; timeout.tv_usec = 0;

        client = accept(server, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);
        FD_ZERO(&set);
        FD_SET(client, &set);

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        inet_ntop(AF_INET, &(client_address.sin_addr), buffer, INET_ADDRSTRLEN);
        address_string = buffer;

        logger.log(address_string+" connected");

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        int res = select(client+1, &set, NULL, NULL, &timeout);
        switch ( res ){
            case 0:
                logger.log(address_string+" connection timed out");
                strcpy(buffer, "TIMEOUT");
                send(client, buffer, 7, 0);
                close(client);
                continue;
            case -1:
                logger.log("Unexpected error occured on "+address_string+" connection");
                close(client);
                continue;
        }
        len = read(client, buffer, CONN_REQ_LEN);

        std::vector<std::string> request = split(buffer, DEL);
        std::string req = buffer;

        if ( request.size() == 2 && request[0] == "JOIN" ) {
            name = request[1];
            Player* player = playerMgr.join(name, client);
            logger.log(address_string+" assigned as "+player->Name);
            std::string temp;
            if ( player->Name == name )
                temp = "OK";
            else {
                temp = "RENAMED";
                temp += DEL+player->Name;
            }

            char cpass[9] = {0};
            pass = player->pass();
            std::memcpy(cpass, &pass, 8);
            player->sendMsg(temp + DEL + cpass);

            temp = "JOIN";
            playerMgr.sendAll(temp+DEL+player->Name);
        } else if ( len == 15 && req[6] == '\n' && req.substr(0, 6) == "REJOIN" ) { 
            std::memcpy(&pass, req.substr(7,8).data(), 8);
            Player* player = playerMgr.rejoin(pass, client);
            if ( player == nullptr ) {
                std::string response = "NOT FOUND";
                send(client, response.data(), response.size(), 0);
                close(client);
            } else {
                logger.log(player->Name+" rejoined from "+address_string);
                player->sendMsg("OK");
            }
        } else {
            std::string response = "BAD REQUEST";
            send(client, response.data(), response.size(), 0);
            close(client);
        }
    }
}

int main ( int argc, char* args[] ) {
    signal(SIGPIPE, SIG_IGN);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if ( server == 0 ) {
        logger.log("ERROR: Failed to create server socket");
        exit(1);
    }
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    address.sin_family      = AF_INET;
    address.sin_port        = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if ( bind(server, (struct sockaddr*)&address, addrlen) < 0 ) {
        logger.log("ERROR: Failed to bind server socket");
        exit(1);
    }
    listen(server, 5);

    std::thread worker_thread(worker);
    std::thread establisher_thread(establisher);

    worker_thread.detach();
    establisher_thread.join();

    return 0;
}
