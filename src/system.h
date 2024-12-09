#ifndef SYSTEM_H
#define SYSTEM_H

enum SystemBit {
    MOVEMENT_SYSTEM_BIT =  (1U << 0),
    RENDER_SYSTEM_BIT =    (1U << 1),
    COLLISION_SYSTEM_BIT = (1U << 2),
    ANIMATION_SYSTEM_BIT = (1U << 3),
    INPUT_SYSTEM_BIT =     (1U << 4),
    TIME_SYSTEM_BIT =      (1U << 5),
    
    INVALID_SYSTEM_BIT =   (1U << 31)
};
typedef enum SystemBit SystemBit;


struct System {
    const char* name;
    SystemBit flag;
    int evaluation_order;
};
typedef struct System System;

extern const System system_table[];

size_t system_table_size();

int system_get_index(SystemBit flag);



#endif
