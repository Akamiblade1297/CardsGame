#ifndef FUNC_CPP
#define FUNC_CPP
#include "main.h"

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

std::random_device rd;
std::default_random_engine rng(rd());

Table table(&rng);

bool isVisible( std::string containerName ) {
    if ( containerName == "TABLE" || containerName == "EQUIPPED" ) { return true; }
    return false;
}

std::string* sStatByName ( std::string name, Player* player ) {
    return nullptr;
}

int* iStatByName ( std::string name, Player* player ) {
    if ( name == "LEVEL" ) {
        return &player->Level;
    } else if ( name == "POWER" ) {
        return &player->Power;
    } else if ( name == "GOLD" ) {
        return &player->Gold;
    } else {
        return nullptr;
    }
}

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
    } else if ( name == "INVENTORY" ) {
        return &player->Inventory;
    } else if ( name == "EQUIPPED" ) {
        return &player->Equiped;
    } else {
        return nullptr;
    }
}

int transformCard(Player* player, Card* card, std::string x, std::string y) { 
    try {
        card->transform(std::stoi(x), std::stoi(y));
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
#endif
