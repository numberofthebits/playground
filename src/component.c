#include "component.h"

#include <core/log.h>

const Component component_table[] = {
    {
        .flag = TRANSFORM_COMPONENT_BIT,
        .size = sizeof(TransformComponent),
        .alignment = alignof(TransformComponent),
        .name = "transform"
    },
    {
        .flag = RENDER_COMPONENT_BIT,
        .size = sizeof(RenderComponent),
        .alignment = alignof(RenderComponent),
        .name = "render" },
    {
        .flag = PHYSICS_COMPONENT_BIT,
        .size = sizeof(PhysicsComponent),
        .alignment = alignof(TransformComponent),
        .name = "physics"
    },
    {
        .flag = ANIMATION_COMPONENT_BIT,
        .size = sizeof(AnimationComponent),
        .alignment = alignof(AnimationComponent),
        .name = "animation"
    },
    {
        .flag = COLLISION_COMPONENT_BIT,
        .size = sizeof(CollisionComponent),
        .alignment = alignof(CollisionComponent),
        .name = "collision"
    }
};

size_t component_table_size() {
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
