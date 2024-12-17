#ifndef _COMPONENT_H
#define _COMPONENT_H

#include <core/math.h>
#include <core/types.h>

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
    COLLISION_COMPONENT_BIT = (1U << 4),
    INPUT_COMPONENT_BIT = (1U << 5),
    TIME_COMPONENT_BIT = (1U << 6),
//    PLAYER_COMPONENT_BIT = (1U << 7),
    
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
    AssetId pipeline_id;
    uint8_t render_layer;
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
    float last_offset;
    uint8_t frames_per_animation_frame;
    uint8_t num_animation_frames;
    uint8_t num_frames_width;
    uint8_t num_frames_height;
    uint8_t is_playing;    
};
typedef struct AnimationComponent_t AnimationComponent;

struct CollisionComponent_t {
    Rectf bounding_rect;
};
typedef struct CollisionComponent_t CollisionComponent;

struct InputComponent_t {
    uint8_t dummy;
};
typedef struct InputComponent_t InputComponent;

struct TimeComponent_t {
    uint64_t created;
    uint64_t expires;
};
typedef struct TimeComponent_t TimeComponent;

// struct PlayerComponent_t {

// };
// typedef struct PlayerComponent_t PlayerComponent;

int component_index(ComponentBit flag);

int component_flag(int index);

size_t component_size(ComponentBit flag);

size_t component_table_size();

const char* component_name(int index);

extern const Component component_table[];



#endif _COMPONENT_H
