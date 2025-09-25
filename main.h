#include <atomic>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <sys/socket.h>

#define CARDS 170
#define TRESH 95
#define BUF   1024

// CARDS CLASSES // 

class Card {
    protected:
        bool Trap;
        uint8_t Number;
    public:
        bool Face;
        int X, Y, Rotation;

        explicit Card ( uint8_t num, bool face = true, int x = 0, int y = 0, int rot = 0 )
            : Face(face), Trap(num<TRESH), Number(num), X(x), Y(y), Rotation(rot) {}

        int getNumber () {
            return Number;
        }
        bool isTrap () {
            return Trap;
        }
        void flip () {
            Face = !Face;            
        }
        void setTransform(int x, int y, int rot = -1) {
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
        void move ( int i, CardContainer* container ) {
            Card* card = Cards[i];
            Cards.erase(Cards.begin()+i);
            container->push(card);
        }
};

class Deck : public CardContainer {
    public:
        Deck () {}
        Deck ( std::vector<Card*> cards ) : CardContainer(cards) {}

        void shuffle (std::default_random_engine rng) {
            std::shuffle(Cards.begin(), Cards.end(), rng);
        }
        void pop_and_move ( CardContainer* container ) {
            Card* card = Cards.back();
            Cards.pop_back();
            container->push(card);
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
                Card card(i);
                TrapDoors.push(&card);
            }
            for ( int i = TRESH ; i < CARDS; i++ ) {
                Card card(i);
                Treasures.push(&card);
            }

            TrapDoors.shuffle(Rng);
            Treasures.shuffle(Rng);
        }
};

// WorkerQueue //

class WorkerQueue {
    private:
        std::mutex QueueMutex;
        std::vector<std::string> Queue;
    public:
        void push ( std::string task ) {
            std::lock_guard<std::mutex> lock(QueueMutex);
            Queue.push_back(task);
        }
        std::string pop () {
            while ( Queue.empty() ) {}

            std::lock_guard<std::mutex> lock(QueueMutex);
            std::string task = Queue.back();
            Queue.pop_back();
            return task;
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
};

class PlayerManager {
    private:
        std::mutex join_mutex;
        static void receiver( Player* player, WorkerQueue* queue ) {
            char buffer[BUF] = {0};
            while ( true ) {
                if ( read(player->ConnectionSocket, buffer, BUF) == 0 ) { break; }
                queue->push(buffer);
                std::fill(buffer, buffer+BUF, 0);
            }
        };
    public:
        std::vector<Player*> Players;
        std::vector<std::thread*> Threads;
        WorkerQueue* Queue;

        PlayerManager (WorkerQueue* queue)
            : Queue(queue) {}

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
            Player new_player(pass, name, socket);
            Players.push_back(&new_player);

            std::thread receiver_thread(receiver, &new_player, Queue);
            receiver_thread.detach();
            return res;
        }
        int rejoin ( uint32_t pass, int socket ) {
            Player* player = playerByPass(pass);
            if ( player == nullptr ) {
                return -1;
            } else {
                player->ConnectionSocket = socket;
                return 0;
            }
        }
};
