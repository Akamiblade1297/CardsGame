#include <iostream>
#include <stdint.h>
#include <deque>
#include <algorithm>
#include <random>
#include <vector>

#define CARDS 170
#define TRESH 95

class Card {
    protected:
        bool Trap;
        uint8_t Number;
    public:
        bool Face;
        static int X, Y, Rotation;

        explicit Card ( uint8_t num, bool face = false, int x = 0, int y = 0, int rot = 0 )
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
        std::deque<Card> Cards;

        CardContainer () {}
        CardContainer ( std::deque<Card> cards )
            : Cards(cards) {}

        void add ( Card card ) {
            Cards.push_front(card);
        }
        void move ( int i, CardContainer* container ) {
            Card card = Cards[i];
            Cards.erase(Cards.begin()+i);
            container->add(card);
        }
};

class Deck : public CardContainer {
    public:
        Deck () {}
        Deck ( std::deque<Card> cards ) : CardContainer(cards) {}

        void shuffle (std::default_random_engine rng) {
            std::shuffle(Cards.begin(), Cards.end(), rng);
        }
};

class Player {
    private:
        uint32_t Id;
    public:
        std::string Name;
        CardContainer Inventory;
        CardContainer Equiped;
};

class Table : public CardContainer {
    private:
        std::random_device Rd;
        std::default_random_engine Rng;
    public:
        std::vector<Player> Players;
        Deck TrapDoors;
        Deck Treasures;

        Table () {
            Rng = std::default_random_engine(Rd());

            for ( int i = 0 ; i < TRESH; i++ ) {
                TrapDoors.add(Card(i));
            }
            for ( int i = TRESH ; i < CARDS; i++ ) {
                Treasures.add(Card(i));
            }

            TrapDoors.shuffle(Rng);
            Treasures.shuffle(Rng);
        }
};
