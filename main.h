#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#define CARDS 170
#define TRESH 95

// CARDS CLASSES // 

class Card {
    protected:
        bool Trap;
        uint8_t Number;
    public:
        bool Face;
        static int X, Y, Rotation;

        explicit Card ( uint8_t num, bool face = true, int x = 0, int y = 0, int rot = 0 )
            : Face(face), Trap(num<TRESH), Number(num) { setTransform(x, y, rot); }

        int getNumber () {
            return Number;
        }
        bool isTrap () {
            return Trap;
        }
        void flip () {
            Face = !Face;            
        }
        void setTransform(int x, int y, int rot = Rotation) {
            X = x;
            Y = y;
            Rotation = rot;
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

// PLAYER CLASSES //

class Player {
    private:
        uint32_t Pass;
    public:
        std::string Name;
        CardContainer Inventory;
        CardContainer Equiped;

        Player ( uint32_t pass, std::string name )
            : Pass(pass), Name(name) {}
};

class PlayerManager {
    public:
        std::vector<Player*> Players;

        PlayerManager () {}

        int join( uint32_t pass, std::string name ) {
            int res = 0;
            for ( Player* player : Players ) {
                if ( player->Name == name ) {
                    name = name+"_ЖалкаяПародия";
                    res = 1;
                }
            }
            Player new_player(pass, name);
            Players.push_back(&new_player);
            return res;
        }
};
