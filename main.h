#include <cstdint>
#include <cstdlib>
#include <iterator>
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
#include <sys/select.h>
#include <arpa/inet.h>

#define CONN_REQ_LEN 25
#define BUF   1024
#define CARDS 170
#define TRESH 95
#define PORT  8494
#define DEL   '\n'

// LOGGING SYSTEM //

class Log {
    private:
        std::ofstream logs;

    public:
        Log ( std::string path ) 
            : logs(path) {}

        /**
         * Logs a message
         *
         * @param msg A message to log
         */
        void log( std::string msg ) {
            std::cout << msg << std::endl;
            logs << msg << std::endl;
        }
        void operator()( std::string msg ) { log(msg); }
};

// CARDS CLASSES // 

/**
 * Card class
 */
class Card {
    public:
        uint8_t Number;
        bool Trap;
        bool Face;
        int X, Y, Rotation;

        /**
         * 
         * @param num A unique card Number
         * @param face Is card Faced or Flopped
         * @param x Coordinate X
         * @param y Coordinate Y
         * @param rot Rotation
         */
        explicit Card ( uint8_t num, bool face = true, int x = 0, int y = 0, int rot = 0 )
            : Face(face), Trap(num<TRESH), Number(num), X(x), Y(y), Rotation(rot) {}

        /**
         * Flip the card
         */
        void flip () {
            Face = !Face;            
        }

        /**
         * Set card position and rotation
         *
         * @param x New coordinate X
         * @param y New coordinate Y
         * @param rot New rotation (-1 to leave unchanged)
         */
        void transform( int x, int y, int rot = -1 ) {
            X = x;
            Y = y;
            if ( rot > 0 ) { Rotation = rot; }
        }
};

/**
 * Base CardContainer class
 */
class CardContainer {
    public:
        std::vector<Card*> Cards;

        CardContainer () {}
        /**
         *
         * @param cards Initial Cards array
         */
        CardContainer ( std::vector<Card*> cards ) : Cards(cards) {}

        ~CardContainer () {
            for ( int i = Cards.size()-1 ; i >= 0 ; i-- ) delete Cards[i];
        }
        /**
         * Pushe the card to container
         *
         * @param card A Pointer to card
         */
        void push ( Card* card ) {
            Cards.push_back(card);
        }

        /**
         * Move card to another CardContainer
         *
         * @param i Card index
         * @param container Destination
         * @return Pointer to moved card if Success, nullptr if Failed (Out of Bounds)
         */
        Card* move ( int i, CardContainer* container ) {
            if ( i < 0 || i >= Cards.size() ) { return nullptr; }
            Card* card = Cards[i];
            Cards.erase(Cards.begin()+i);
            container->push(card);
            return card;
        }

        /**
         * Get a card by index
         *
         * @param i Card index
         * @return Pointer to card if Success, nullptr if Failed (Out of Bounds)
         */
        Card* card ( int i ) {
            if ( i < 0 || i >= Cards.size() ) { return nullptr; }
            return Cards[i];
        }

        /**
         * Change card position and rotation
         *
         * @param i Card index
         * @param x New coordinate X
         * @param y New coordinate Y
         * @param rot New rotation (-1 for default)
         * @return 0 if Success, -1 if Error (Out of Bounds)
         */
        int transform ( int i, int x, int y, int rot = -1 ) {
            if ( i < 0 || i >= Cards.size() ) { return -1; }
            Cards[i]->transform(x, y, rot);
            return 0;
        }
};

/**
 * Deck class
 */
class Deck : public CardContainer {
    private:
        std::default_random_engine* rng;
    public:
        /**
         *
         * @param random_engine A random engine used for shuffling
         */
        Deck ( std::default_random_engine* random_engine ) 
            : rng(random_engine) {}
        /**
         *
         * @param random_engine A random engine used for shuffling
         * @param cards Inital Cards array
         */
        Deck ( std::default_random_engine* random_engine, std::vector<Card*> cards ) 
            : rng(random_engine), CardContainer(cards) {}

        /**
         * Shuffle the deck
         */
        void shuffle () {
            std::shuffle(Cards.begin(), Cards.end(), *rng);
        }

        /**
         * Pop the card and Move it to other container
         *
         * @param container Destination
         * @return Pointer to a card if Success, nullptr if Failed (Empty)
         */
        Card* pop_and_move ( CardContainer* container ) {
            if ( Cards.empty() ) { return nullptr; }

            Card* card = Cards.back();
            Cards.pop_back();
            container->push(card);
            return card;
        }
};

/**
 * Table class
 */
class Table : public CardContainer {
    private:
        std::default_random_engine *Rng;
    public:
        Deck TrapDoors;
        Deck Treasures;

        /**
         * @param rng Random Number Generator for Shuffling
         */
        Table ( std::default_random_engine* rng ) : Rng(rng), TrapDoors(Rng), Treasures(Rng) {
            for ( int i = 0 ; i < TRESH; i++ ) {
                Card* card = new Card(i);
                TrapDoors.push(card);
            }
            for ( int i = TRESH ; i < CARDS; i++ ) {
                Card* card = new Card(i);
                Treasures.push(card);
            }
            
            TrapDoors.shuffle();
            Treasures.shuffle();
        }
};

// PLAYER CLASSES //

/**
 * Player class
 */
class Player {
    private:
        uint64_t Pass;
    public:
        int ConnectionSocket;
        std::string Name;
        int Level;
        int Power;
        int Gold;
        CardContainer Inventory;
        CardContainer Equiped;

        /**
         *
         * @param pass Secert key to authorize user in case connection lost
         * @param name Username
         * @param socket Player connection File Descriptor
         */
        Player ( uint64_t pass, std::string name, int socket )
            : Pass(pass), Name(name), ConnectionSocket(socket) {}

        /**
         * Get player Secret key
         * */
        uint64_t pass () {
            return Pass;
        }

        /**
         * Send message to Player
         *
         * @param message Message to send
         * @param n Message length (-1 for automatic)
         */
        void sendMsg( std::string message, int n = -1 ) {
            message += DEL;
            if ( n == -1 ) n = message.length();
            send(ConnectionSocket, message.data(), n, 0);
        }
};

// WorkerQueue //

/**
 * Worker Queue class
 */
class WorkerQueue {
    private:
        std::mutex QueueMutex;
        std::vector<std::tuple<Player*, std::string>> Queue;
    public:
        /**
         * Push a Player request to a Queue
         *
         * @param player Request sender
         * @param request Request string
         */
        void push ( Player* player, std::string request ) {
            std::lock_guard<std::mutex> lock(QueueMutex);
            std::tuple<Player*, std::string> task(player, request);
            Queue.push_back(task);
        }

        /**
         * Pop a request from Queue
         *
         * @return Tuple (Player sender, String request)
         */
        std::tuple<Player*, std::string> pop () {
            while ( Queue.empty() ) {}

            std::lock_guard<std::mutex> lock(QueueMutex);
            std::tuple<Player*, std::string> task = Queue.back();
            Queue.pop_back();
            return task;
        }
};

/**
 * Player Manager class
 */
class PlayerManager {
    private:
        std::vector<Player*> Players;
        WorkerQueue* Queue;
        Log* Logger;
        std::mutex join_mutex;
        std::random_device& rd;

        static void receiver ( Player* player, WorkerQueue* queue, Log* logger ) {
            char buffer[BUF] = {0};
            while ( true ) {
                if ( read(player->ConnectionSocket, buffer, BUF) == 0 ) { logger->log(player->Name+" disconnected"); break; }
                queue->push(player, buffer);
                std::fill(buffer, buffer+BUF, 0);
            }
        };
        uint64_t gen_pass () {
            uint64_t new_pass = rd();
            new_pass = (new_pass<<32) + rd();
            return new_pass;
        }
    public:
        /**
         *
         * @param queue Pointer to a requests Queue
         * @param logger Pointer to a logging object
         * @param rd Reference to a Random device to generate Secret Keys
         */
        PlayerManager ( WorkerQueue* queue, Log* logger, std::random_device& rd )
            : Queue(queue), Logger(logger), rd(rd) {}
        ~PlayerManager () {
            for ( int i = Players.size()-1 ; i >= 0 ; i-- ) delete Players[i];
        }

        size_t size () const {
            return Players.size();
        }

        /**
         * Get Player by Id
         * 
         * @param id Player Id
         * @return Pointer to a Player if Success, nullptr if Failed (Out of Bounds)
         */
        Player* playerById( size_t id ) {
            if ( id <= 0 or id > Players.size() ) {
                return nullptr;
            } else {
                return Players[id-1];
            }
        }

        /**
         * Get Player by Username
         *
         * @param name Player Username
         * @return Pointer to a Player if Success, nullptr if Failed (Not Found)
         */
        Player* playerByName( std::string name ) {
            for ( Player* player : Players ) {
                if ( player->Name == name ) {
                    return player;
                }
            }
            return nullptr;
        }

        /**
         * Get Player by Pass
         *
         * @param pass Player Secret key
         * @return Pointer to a Player if Success, nullptr if Failed (Not Found)
         */
        Player* playerByPass( uint64_t pass ) {
            for ( Player* player : Players ) {
                if ( pass == player->pass() ) {
                    return player;
                }
            }
            return nullptr;
        }

        /**
         * Get Id of a Player
         *
         * @param player Pointer to a player
         * @return Player Id if Success, -1 if Failed (Not Found)
         */
        size_t playerId( const Player* player ) {
            for ( int i = 0 ; i < Players.size() ; i++ ) {
                if ( Players[i] == player ) { return i+1; }
            }
            return -1;
        }

        /**
         * Add Player to a PlayerManager
         *
         * @param pass New Player Secret key
         * @param name New Player Username
         * @param socket New Player connection File Descriptor
         * @return Pointer to a New Player
         */
        Player* join( std::string name, const int socket ) {
            std::lock_guard<std::mutex> lock(join_mutex);
            for ( Player* player : Players ) {
                if ( player->Name == name ) {
                    name = name+"_ЖалкаяПародия";
                }
            }
            Player* new_player = new Player(gen_pass(), name, socket);
            Players.push_back(new_player);

            std::thread receiver_thread(receiver, new_player, Queue, Logger);
            receiver_thread.detach();

            return new_player;
        }

        /**
         * Assign new connection for Player
         *
         * @param pass Player Secret key
         * @param socket New Player connection File Descriptor
         * @return Pointer to a Player if Success, nullptr if Failed (Not Found)
         */
        Player* rejoin ( uint64_t pass, const int socket ) {
            Player* player = playerByPass(pass);
            if ( player != nullptr ) {
                player->ConnectionSocket = socket;
                std::thread receiver_thread(receiver, player, Queue, Logger);
                receiver_thread.detach();
            }
            return player;
        }

        /**
         * Send a message to Player by Username
         *
         * @param name Player Username
         * @param message Message to send
         * @param n Message length (-1 for automatic)
         * @return 0 if Success, -1 if Failed (Not Found)
         */
        int sendByName ( std::string name, std::string message, int n = -1 ) {
            Player* player = playerByName(name);
            if ( player == nullptr ) { return -1; }

            player->sendMsg(message, n);
            return 0;
        }

        /**
         * Send message to all Players
         *
         * @param message Message to send
         * @param n Message length (-1 for automatic)
         */
        void sendAll ( std::string message, int n = -1 ) {
            for ( Player* player : Players ) {
                player->sendMsg(message, n);
            }
        }
};

// FUNCTION PROTOTYPES //

/**
 * Get Player's string Stat by Name
 *
 * @param name Stat name
 * @param player Player
 * @return Pointer to a Stat if Success, nullptr if Failed (Not Found)
 */
std::string* sStatByName ( std::string name, Player* player );
/**
 * Get Player's int Stat by Name
 *
 * @param name Stat name
 * @param player Player
 * @return Pointer to a Stat if Success, nullptr if Failed (Not Found)
 */
int* iStatByName ( std::string name, Player* player );
/**
 * Get CardContainer by Name
 *
 * @param name Container Name
 * @param player Player, that might own the Container
 * @return Pointer to a Container if Success, nullptr if Failed (Not Found)
 */
CardContainer* containerByName ( std::string name, Player* player );
/**
 * Get Spatial CardContainer by Name
 *
 * @param name Container Name
 * @param player Player, that might own the Container
 * @return Pointer to a Container if Success, nullptr if Failed (Not Found)
 */
CardContainer* spatialByName   ( std::string name, Player* player );
/**
 * Get Deck CardContainer by Name
 *
 * @param name Container Name
 * @param player Player, that might own the Container
 * @return Pointer to a Container if Success, nullptr if Failed (Not Found)
 */
Deck* deckByName ( std::string name );
/**
 * Get a Visibility of Card Container
 *
 * @param containerName Container Name
 * @return True if Container if visible, False otherwise
 */
bool isVisible ( std::string containerName );
/**
 * Transform Card
 *
 * @param player Player that should get response
 * @param card a Card to transform
 * @param x X coordinate
 * @param y Y coordinate
 * @return 0 if Success, -1 if Failed
 */
int transformCard(Player* player, Card* card, std::string x, std::string y);
/**
 * Transform Card inside a Container
 *
 * @param player Player that should get response
 * @param container Container, containing a card
 * @param i Card Index
 * @param x X coordinate
 * @param y Y coordinate
 * @return 0 if Success, -1 if Failed
 */
int transformContainer(Player* player, CardContainer* container, std::string i, std::string x, std::string y);
