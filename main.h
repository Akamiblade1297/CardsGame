#include <iostream>
#include <stdint.h>
#include <deque>
#include <algorithm>
#include <random>

class Card {
    protected:
        bool Trap;
        uint8_t Number;
    public:
        bool Face;
        static int X, Y, Rotation;

        explicit Card ( uint8_t num, bool face = false, int x = 0, int y = 0, int rot = 0 )
            : Face(face), Trap(num<95), Number(num) { setTransform(x, y, rot); }

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
    private:
        std::random_device Rd;
        std::default_random_engine Rng;
    public:
        Deck ( std::deque<Card> cards ) : CardContainer(cards) {
            Rng = std::default_random_engine(Rd());
        }

        void shuffle () {
            std::shuffle(Cards.begin(), Cards.end(), Rng);
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
