// Disable implicit fallthrough warning
#include "game.h"

int main() {
    game::init();
    game::run();
    game::destroy();
    return 0;
}
