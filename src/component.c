#include "component.h"

#include <core/log.h>

const Component component_table[] = {
    {
        .flag = TRANSFORM_COMPONENT_BIT,
        .size = sizeof(TransformComponent),
        .alignment = alignof(TransformComponent),
        .name = "TransformComponent"
    },
    {
        .flag = RENDER_COMPONENT_BIT,
        .size = sizeof(RenderComponent),
        .alignment = alignof(RenderComponent),
        .name = "RenderComponent" },
    {
        .flag = PHYSICS_COMPONENT_BIT,
        .size = sizeof(PhysicsComponent),
        .alignment = alignof(TransformComponent),
        .name = "PhysicsComponent"
    },
    {
        .flag = ANIMATION_COMPONENT_BIT,
        .size = sizeof(AnimationComponent),
        .alignment = alignof(AnimationComponent),
        .name = "AnimationComponent"
    },
    {
        .flag = COLLISION_COMPONENT_BIT,
        .size = sizeof(CollisionComponent),
        .alignment = alignof(CollisionComponent),
        .name = "CollisionComponent"
    },
    {
        .flag = INPUT_COMPONENT_BIT,
        .size = sizeof(InputComponent),
        .alignment = alignof(InputComponent),
        .name = "InputComponent"
    },
    {
        .flag = TIME_COMPONENT_BIT,
        .size = sizeof(TimeComponent),
        .alignment = alignof(TimeComponent),
        .name = "TimeComponent"
    }
    /* { */
    /*     .flag = PLAYER_COMPONENT_BIT, */
    /*     .size = sizeof(PlayerComponent), */
    /*     .alignment = alignof(PlayerComponent), */
    /*     .name = "PlayerComponent" */
    /* } */
};

int component_table_size(void) {
    return sizeof(component_table) / sizeof(Component);
}

int component_index(ComponentBit flag) {
    for (int i = 0; i < component_table_size(); ++i) {
        if (component_table[i].flag == flag) {
            return i;
        }
    }

    return -1;
}

ComponentBit component_flag(int index) {
    if (index >= component_table_size()) {
        return INVALID_COMPONENT_BIT;
    }
    
    return component_table[index].flag;
}

size_t component_size(ComponentBit flag) {
    int index = component_index(flag);
    if (index < 0) {
        LOG_WARN("Failed to find component index for bit %d\n", flag);
        return -1;
    }

    return component_table[index].size;
}

const char* component_name(int index) {
    return component_table[index].name;
}
