#include "systems.h"

const System system_table[] = {
    {
        .name = "movement",
        .flag = MOVEMENT_SYSTEM_BIT,
        .evaluation_order = 0,
    },
    {
        .name = "render",
        .flag = RENDER_SYSTEM_BIT,
        .evaluation_order = 0,
    },
    {
        .name = "collision",
        .flag = RENDER_SYSTEM_BIT,
        .evaluation_order = 0,
    },
    {
        .name = "animation",
        .flag = ANIMATION_SYSTEM_BIT,
        .evaluation_order = 0
    },
    {
        .name = "input",
        .flag = INPUT_SYSTEM_BIT,
        .evaluation_order = 0,
    },
    {
        .name = "time",
        .flag = TIME_SYSTEM_BIT,
        .evaluation_order = 0
    },
    {
        .name = "player",
        .flag = PLAYER_SYSTEM_BIT,
        .evaluation_order = 0
    }
};

size_t system_table_size() {
    return sizeof(system_table) / sizeof(System);
}

int system_get_index(SystemBit flag) {
    for (size_t i = 0; i < system_table_size(); ++i) {
        if (system_table[i].flag == flag) {
            return (int)i;
        }
    }

    return -1;
}

