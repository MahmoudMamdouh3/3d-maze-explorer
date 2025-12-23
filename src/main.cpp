#include "Core/Game.h"
#include <iostream>

int main() {
    try {
        Game game;
        game.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}