#ifndef _COMPONENT_H
#define _COMPONENT_H

#include "math.h"
#include "types.h"

#include <stdalign.h>


// Code smell. The user should be able to define this
// and he can, as ComponentBit is just an int, but
// what about the actual data type the enum represents?
// registry_add_component takes void* so that's okay, but
// didn't check the rest of them, or even what "the rest" is.
enum ComponentBit {
    TRANSFORM_COMPONENT_BIT = (1U << 0),
    RENDER_COMPONENT_BIT = (1U << 1),
    PHYSICS_COMPONENT_BIT = (1U << 2),
    ANIMATION_COMPONENT_BIT = (1U << 3),
    INVALID_COMPONENT_BIT = (1U << 31)
};
typedef enum ComponentBit ComponentBit;

struct Component_t {
    ComponentBit flag;
    size_t size;
    size_t alignment;
    const char* name;
};
typedef struct Component_t Component;

int component_index(ComponentBit flag);

int component_flag(int index);

size_t component_size(ComponentBit flag);

const char* component_name(int index);


struct Transform_Component_t {
    Vec3f pos;
    Vec2f scale;
    float rotation; // x/y-plane rotation for now
};
typedef struct Transform_Component_t TransformComponent;

typedef enum RenderComponentFlags_t {
    Position = (1 << 0),
    UV = (1 << 1),
    Normal = (1 << 2),
    Color = (1 << 3),
    Texture = (1 << 4),
    TileMap = (1 << 5),
} RenderComponentFlags;

typedef struct {
    // Normalized texture coordinate modifier
    // Remember. In GL, positive Y is up. In image
    // space, positive Y is down.
    Vec2f tex_coord_offset;
    Vec2f tex_coord_scale;
    AssetId material_id;
    uint8_t render_layer;
    AssetId pipeline_id;
} RenderComponent;

// inline void render_component_flags_set(RenderComponent* component, RenderComponentFlags flags) {
//     component->flags |= flags;
// }

// inline void render_component_flags_clear(RenderComponent* component, RenderComponentFlags flags) {
//     component->flags &= ~(flags);
// }

struct Physics_Component_t {
    Vec2f velocity;
};
typedef struct Physics_Component_t PhysicsComponent;

struct AnimationComponent_t {
    uint8_t frames_per_animation_frame;
    uint8_t num_animation_frames;
    uint8_t is_playing;
};
typedef struct AnimationComponent_t AnimationComponent;

static Component component_table[] = {
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
    }
};

static size_t component_table_size = sizeof(component_table) / sizeof(Component);


#endif _COMPONENT_H
