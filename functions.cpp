#include "main.h"

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
