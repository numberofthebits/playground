#ifndef SYSTEM_H
#define SYSTEM_H

#include <stddef.h>

enum SystemBit {
    SYSTEM_BIT_ITER_BEGIN =      (1U << 0),
    
    MOVEMENT_SYSTEM_BIT =        (1U << 0),
    RENDER_SYSTEM_BIT =          (1U << 1),
    COLLISION_SYSTEM_BIT =       (1U << 2),
    ANIMATION_SYSTEM_BIT =       (1U << 3),
    INPUT_SYSTEM_BIT =           (1U << 4),
    TIME_SYSTEM_BIT =            (1U << 5),
    PLAYER_SYSTEM_BIT =          (1U << 6),
    CAMERA_MOVEMENT_SYSTEM_BIT = (1U << 7),

    SYSTEM_BIT_ITER_END   =      (1U << 8),
    
    INVALID_SYSTEM_BIT =   (1U << 30)
};
typedef enum SystemBit SystemBit;


struct System {
    const char* name;
    SystemBit flag;
    int evaluation_order;
};
typedef struct System System;

extern const System system_table[];

size_t system_table_size(void);

int system_get_index(SystemBit flag);



#endif
