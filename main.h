#include <numeric>
#include <typeinfo>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CONN_REQ_LEN 25
#define BUF   1024
#define CARDS 170
#define TRESH 95
#define PORT  8494

// LOGGING SYSTEM //

class Log {
    private:
        std::ofstream logs;

    public:
        Log ( std::string path ) 
            : logs(path) {}

        void operator()( std::string msg ) {
            std::cout << msg << std::endl;
            logs << msg << std::endl;
        }
        void log( std::string msg ) { operator()(msg); }
};

// CARDS CLASSES // 

class Card {
    public:
        uint8_t Number;
        bool Trap;
        bool Face;
        int X, Y, Rotation;

        explicit Card ( uint8_t num, bool face = true, int x = 0, int y = 0, int rot = 0 )
            : Face(face), Trap(num<TRESH), Number(num), X(x), Y(y), Rotation(rot) {}

        void flip () {
            Face = !Face;            
        }
        void transform( int x, int y, int rot = -1 ) {
            X = x;
            Y = y;
            if ( rot > 0 ) { Rotation = rot; }
        }
};

class CardContainer {
    public:
        std::vector<Card*> Cards;

        CardContainer () {}
        CardContainer ( std::vector<Card*> cards )
            : Cards(cards) {}

        void push ( Card* card ) {
            Cards.push_back(card);
        }
        Card* move ( int i, CardContainer* container ) {
            if ( i < 0 || i >= Cards.size() ) { return nullptr; }
            Card* card = Cards[i];
            Cards.erase(Cards.begin()+i);
            container->push(card);
            return card;
        }
        Card* card ( int i ) {
            if ( i < 0 || i >= Cards.size() ) { return nullptr; }
            return Cards[i];
        }
        int transform ( int i, int x, int y, int rot = -1 ) {
            if ( i < 0 || i >= Cards.size() ) { return -1; }
            Cards[i]->transform(x, y, rot);
            return 0;
        }
};

class Deck : public CardContainer {
    public:
        Deck () {}
        Deck ( std::vector<Card*> cards ) : CardContainer(cards) {}

        void shuffle (std::default_random_engine rng) {
            std::shuffle(Cards.begin(), Cards.end(), rng);
        }
        Card* pop_and_move ( CardContainer* container ) {
            if ( Cards.empty() ) { return nullptr; }

            Card* card = Cards.back();
            Cards.pop_back();
            container->push(card);
            return card;
        }
};

class Table : public CardContainer {
    private:
        std::random_device Rd;
        std::default_random_engine Rng;
    public:
        Deck TrapDoors;
        Deck Treasures;

        Table () {
            Rng = std::default_random_engine(Rd());

            for ( int i = 0 ; i < TRESH; i++ ) {
                Card* card = new Card(i);
                TrapDoors.push(card);
            }
            for ( int i = TRESH ; i < CARDS; i++ ) {
                Card* card = new Card(i);
                Treasures.push(card);
            }
            
            shuffleTrapDoors();
            shuffleTreasures();
        }

        void shuffleTrapDoors() {
            TrapDoors.shuffle(Rng);
        }
        void shuffleTreasures() {
            Treasures.shuffle(Rng);
        }
};

// PLAYER CLASSES //

class Player {
    private:
        uint32_t Pass;
    public:
        int ConnectionSocket;
        std::string Name;
        CardContainer Inventory;
        CardContainer Equiped;

        Player ( uint32_t pass, std::string name, int socket )
            : Pass(pass), Name(name), ConnectionSocket(socket) {}

        bool auth ( uint32_t pass ) {
            return ( Pass == pass );
        }
        void sendMsg( std::string message ) {
            int n = message.length();
            char buffer[n];
            strcpy(buffer, message.data());
            send(ConnectionSocket, buffer, n, 0);
        }
};

// WorkerQueue //

class WorkerQueue {
    private:
        std::mutex QueueMutex;
        std::vector<std::tuple<Player*, std::string>> Queue;
    public:
        void push ( Player* player, std::string request ) {
            std::lock_guard<std::mutex> lock(QueueMutex);
            std::tuple<Player*, std::string> task(player, request);
            Queue.push_back(task);
        }
        std::tuple<Player*, std::string> pop () {
            while ( Queue.empty() ) {}

            std::lock_guard<std::mutex> lock(QueueMutex);
            std::tuple<Player*, std::string> task = Queue.back();
            Queue.pop_back();
            return task;
        }
};

class PlayerManager {
    private:
        WorkerQueue* Queue;
        Log* Logger;
        std::mutex join_mutex;

        static void receiver( Player* player, WorkerQueue* queue, Log* logger ) {
            char buffer[BUF] = {0};
            while ( true ) {
                if ( read(player->ConnectionSocket, buffer, BUF) == 0 ) { logger->log(player->Name+" disconnected"); break; }
                queue->push(player, buffer);
                std::fill(buffer, buffer+BUF, 0);
            }
        };
    public:
        std::vector<Player*> Players;

        PlayerManager ( WorkerQueue* queue, Log* logger )
            : Queue(queue), Logger(logger) {}

        Player* playerById( int id ) {
            if ( id <= 0 or id > Players.size() ) {
                return nullptr;
            } else {
                return Players[id-1];
            }
        }
        Player* playerByName( std::string name ) {
            for ( Player* player : Players ) {
                if ( player->Name == name ) {
                    return player;
                }
            }
            return nullptr;
        }
        Player* playerByPass( uint32_t pass ) {
            for ( Player* player : Players ) {
                if ( player->auth(pass) ) {
                    return player;
                }
            }
            return nullptr;
        }
        int playerId( Player* player ) {
            for ( int i = 0 ; i < Players.size() ; i++ ) {
                if ( Players[i] == player ) { return i+1; }
            }
            return -1;
        }
        int join( uint32_t pass, std::string name, int socket ) {
            int res = 0;
            std::lock_guard<std::mutex> lock(join_mutex);
            for ( Player* player : Players ) {
                if ( player->Name == name ) {
                    name = name+"_ЖалкаяПародия";
                    res = 1;
                }
            }
            Player* new_player = new Player(pass, name, socket);
            Players.push_back(new_player);

            std::thread receiver_thread(receiver, new_player, Queue, Logger);
            receiver_thread.detach();
            return res;
        }
        Player* rejoin ( uint32_t pass, int socket ) {
            Player* player = playerByPass(pass);
            if ( player == nullptr ) {
                return nullptr;
            } else {
                player->ConnectionSocket = socket;
                std::thread receiver_thread(receiver, player, Queue, Logger);
                receiver_thread.detach();
                return player;
            }
        }
        int sendByName ( std::string name, std::string message ) {
            Player* player = playerByName(name);
            if ( player == nullptr ) { return -1; }

            player->sendMsg(message);
            return 0;
        }
        void sendAll ( std::string message ) {
            int n = message.length();
            char buffer[n];
            strcpy(buffer, message.data());
            for ( Player* player : Players ) {
                send(player->ConnectionSocket, buffer, n, 0);
            }
        }
};
