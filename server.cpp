#include "main.h"

Table table;

CardContainer* containerByName ( std::string name, Player* player ) {
    if ( name == "TABLE" ) {
        return &table;
    } else if ( name == "TREASURES" ) {
        return &table.Treasures;
    } else if ( name == "TRAPDOORS" ) {
        return &table.TrapDoors;
    } else if ( name == "INVENTORY" ) {
        return &player->Inventory;
    } else if ( name == "EQUIPPED" ) {
        return &player->Equiped;
    } else {
        return nullptr;
    }
}

Deck* deckByName ( std::string name ) {
    if ( name == "TREASURES" ) {
        return &table.Treasures;
    } else if ( name == "TRAPDOORS" ) {
        return &table.TrapDoors;
    } else {
        return nullptr;
    }
}

CardContainer* spatialByName ( std::string name, Player* player ) {
    if ( name == "TABLE" ) {
        return &table;
        return &table.TrapDoors;
    } else if ( name == "INVENTORY" ) {
        return &player->Inventory;
    } else if ( name == "EQUIPPED" ) {
        return &player->Equiped;
    } else {
        return nullptr;
    }
}

WorkerQueue queue;
Log logger("logs/log");
PlayerManager playerMgr(&queue, &logger);
struct sockaddr_in address;
int addrlen = sizeof(address);
int server;
int reuse = 1;
char del = '\n';

std::vector<std::string> split( char* str, char del ) {
    std::vector<std::string> str_vec;
    std::stringstream str_stream(str);
    std::string s;
    
    while ( std::getline(str_stream, s, del) ) {
        str_vec.push_back(s);
    }

    return str_vec;
}

int transformCard(Player* player, Card* card, std::string x, std::string y) { 
    try {
        card->transform(std::stoi(x), std::stoi(y));
        player->sendMsg("OK");
        return 0;
    } catch ( std::invalid_argument ) {
        player->sendMsg("NOT NUMBER");
        return -1;
    } catch ( std::out_of_range ) {
        player->sendMsg("OUT OF RANGE");
        return -1;
    }
}

int transformContainer(Player* player, CardContainer* container, std::string i, std::string x, std::string y) { 
    try {
        if ( container->transform(std::stoi(i), std::stoi(x), std::stoi(y)) == 0 ) {
            player->sendMsg("OK");
            return 0;
        } else {
            player->sendMsg("NOT FOUND");
            return -1;
        }
    } catch ( std::invalid_argument ) {
        player->sendMsg("NOT NUMBER");
        return -1;
    } catch ( std::out_of_range ) {
        player->sendMsg("OUT OF RANGE");
        return -1;
    }
}

void worker () {
    char buffer[BUF];
    while ( true ) {
        std::fill(buffer, buffer+BUF, 0);
        auto [player, req] = queue.pop();
        strcpy(buffer, req.c_str());
        std::vector<std::string> request = split(buffer, del);
        
        if ( request.size() == 2 && request[0] == "CHAT" ) { // CHAT <message>
            std::string temp = "CHAT";
            playerMgr.sendAll(temp + del + player->Name + del + request[1]);
            logger.log(player->Name+": "+request[1]);
        } else if ( request.size() == 3 && request[0] == "WHISPER" ) { // WHISPER <player> <message>
            Player* player_dest = playerMgr.playerByName(request[1]);
            std::string temp = "WHISPER";
            if ( playerMgr.sendByName(request[1], temp + del + player->Name + del + request[2]) == 0 ) {
                player->sendMsg("OK");
                logger.log(player->Name+" -> "+request[1]+": "+request[2]);
            } else {
                player->sendMsg("NOT FOUND");
            }
        } else if ( request.size() == 5 && request[0] == "DECK" ) { // DECK <src> <dest> <x> <y>
            Deck* src           = deckByName(request[1]);
            CardContainer* dest = containerByName(request[2], player);
            Card* card = nullptr;
            if ( src == dest || src == nullptr || dest == nullptr ) {
                player->sendMsg("ERROR");
            } else {
                card = src->pop_and_move(dest);
                if ( card == nullptr ) {
                    player->sendMsg("EMPTY");
                } else if ( transformCard(player, card, request[3], request[4]) == 0 ) {
                    logger.log(player->Name+" moved Card_"+std::to_string(card->Number)+" from "+request[1]+" to "+request[2]+" (X: "+request[3]+", Y: "+request[4]+")");
                }
            }
        } else if ( ( request.size() == 6 ) && request[0] == "MOVE" ) { // MOVE <src> <card> <dest> <x> <y>
            CardContainer* src  = spatialByName(request[1], player);
            CardContainer* dest = containerByName(request[3], player);
            Card* card;

            if ( src == nullptr || dest == nullptr ) {
                player->sendMsg("ERROR");
                continue;
            }
            else if ( src == dest ) {
                if ( transformContainer(player, src, request[2], request[4], request[5]) != 0 ) {
                    continue;
                } else {
                    card = src->Cards[std::stoi(request[2])];
                }
            } else {
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
                card = src->move(i, dest);
                if ( card == nullptr ) {
                    player->sendMsg("INVALID CARD");
                    continue;
                } else {
                    transformCard(player, card, request[4], request[5]);
                }
            }
            logger.log(player->Name+" moved Card_"+std::to_string(card->Number)+" from "+request[1]+" to "+request[3]+" (X: "+request[4]+", Y: "+request[5]+")");
        }
    }
}

void establisher () {
    std::string name;
    std::string address_string;
    char buffer[CONN_REQ_LEN] = {0};
    struct sockaddr_in client_address;
    uint32_t pass;
    int client;
    while ( true ) {
        client = accept(server, (struct sockaddr*)&client_address, (socklen_t*)&addrlen);

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        inet_ntop(AF_INET, &(client_address.sin_addr), buffer, INET_ADDRSTRLEN);
        address_string = buffer;

        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
        read(client, buffer, CONN_REQ_LEN);

        std::vector<std::string> request = split(buffer, del);

        if ( request.size() == 3 && request[0] == "JOIN") {
            std::memcpy(&pass, request[1].data(), 4);
            name = request[2];
            switch ( playerMgr.join(pass, name, client) ) {
                case 0:
                    logger.log(address_string+" assigned as "+name);
                    strcpy(buffer, "OK");
                    send(client, buffer, 2, 0);
                    break;
                case 1:
                    Player* player = playerMgr.playerByPass(pass);
                    logger.log(address_string+" assigned as "+player->Name);
                    strcpy(buffer, "RENAMED");
                    send(client, buffer, 7, 0);
                    break;
            }
        } else if ( request.size() == 2 && request[0] == "REJOIN" ) { 
            std::memcpy(&pass, request[1].data(), 4);
            Player* player = playerMgr.rejoin(pass, client);
            if ( player == nullptr ) {
                strcpy(buffer, "NOT FOUND");
                send(client, buffer, 9, 0);
                close(client);
            } else {
                logger.log(player->Name+" rejoined from "+address_string);
                strcpy(buffer, "OK");
                send(client, buffer, 2, 0);
            }
        } else {
            strcpy(buffer, "UNKNOWN");
            send(client, buffer, 7, 0);
            close(client);
        }
        std::fill(buffer, buffer+CONN_REQ_LEN, 0);
    }
}

int main () {
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
