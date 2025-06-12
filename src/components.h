#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "core/componentbase.h"
#include "core/math.h"
#include "core/os.h"
#include "core/types.h"

#include <stdalign.h>

enum ComponentBit {
  TRANSFORM_COMPONENT_BIT = (1U << 0),
  RENDER_COMPONENT_BIT = (1U << 1),
  PHYSICS_COMPONENT_BIT = (1U << 2),
  ANIMATION_COMPONENT_BIT = (1U << 3),
  COLLISION_COMPONENT_BIT = (1U << 4),
  INPUT_COMPONENT_BIT = (1U << 5),
  TIME_COMPONENT_BIT = (1U << 6),
  CAMERA_MOVEMENT_COMPONENT_BIT = (1U << 7),
  PROJECTILE_EMITTER_COMPONENT_BIT = (1U << 8),
  PROJECTILE_COMPONENT_BIT = (1U << 9),
  HEALTH_COMPONENT_BIT = (1U << 10)
};
typedef enum ComponentBit ComponentBit;

struct Transform_Component_t {
  Vec3f pos;
  Vec2f scale;
  float rotation; // x/y-plane rotation for now
};
typedef struct Transform_Component_t TransformComponent;

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

// inline void render_component_flags_set(RenderComponent* component,
// RenderComponentFlags flags) {
//     component->flags |= flags;
// }

// inline void render_component_flags_clear(RenderComponent* component,
// RenderComponentFlags flags) {
//     component->flags &= ~(flags);
// }

struct Physics_Component_t {
  // TODO: Replace this with two scalars:
  //       angle and velocity.
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
  // axis aligned bounding rect
  // When paired with TransformComponent, pos should
  // act as an offset, so generally leave pos at 0 in this case
  Rectf aabr;
};
typedef struct CollisionComponent_t CollisionComponent;

struct InputComponent_t {
  uint8_t dummy;
};
typedef struct InputComponent_t InputComponent;

struct TimeComponent_t {
  TimeT created;
  TimeT expires;
};
typedef struct TimeComponent_t TimeComponent;

typedef struct {
  uint8_t dummy;
} CameraMovementComponent;

typedef struct {
  TimeT last_emitted;
  TimeT emission_frequency;
  TimeT projectile_duration;
  int32_t damage;
} ProjectileEmitterComponent;

typedef struct {
  int32_t damage;
} ProjectileComponent;

typedef struct {
  int32_t health;
} HealthComponent;

// Our "user defined" component table. We feed this
// to the entity component system, so that it has a way
// to differentiate components, and to know the size and
// alignment for allocating the component pools. Nifty.
static const struct Component component_table[] = {
    {.flag = TRANSFORM_COMPONENT_BIT,
     .size = sizeof(TransformComponent),
     .alignment = alignof(TransformComponent),
     .name = "TransformComponent"},

    {.flag = RENDER_COMPONENT_BIT,
     .size = sizeof(RenderComponent),
     .alignment = alignof(RenderComponent),
     .name = "RenderComponent"},

    {.flag = PHYSICS_COMPONENT_BIT,
     .size = sizeof(PhysicsComponent),
     .alignment = alignof(TransformComponent),
     .name = "PhysicsComponent"},

    {.flag = ANIMATION_COMPONENT_BIT,
     .size = sizeof(AnimationComponent),
     .alignment = alignof(AnimationComponent),
     .name = "AnimationComponent"},

    {.flag = COLLISION_COMPONENT_BIT,
     .size = sizeof(CollisionComponent),
     .alignment = alignof(CollisionComponent),
     .name = "CollisionComponent"},

    {.flag = INPUT_COMPONENT_BIT,
     .size = sizeof(InputComponent),
     .alignment = alignof(InputComponent),
     .name = "InputComponent"},

    {.flag = TIME_COMPONENT_BIT,
     .size = sizeof(TimeComponent),
     .alignment = alignof(TimeComponent),
     .name = "TimeComponent"},

    {.flag = CAMERA_MOVEMENT_COMPONENT_BIT,
     .size = sizeof(CameraMovementComponent),
     .alignment = alignof(CameraMovementComponent),
     .name = "CameraMovementComponent"},

    {.flag = PROJECTILE_EMITTER_COMPONENT_BIT,
     .size = sizeof(ProjectileEmitterComponent),
     .alignment = alignof(ProjectileEmitterComponent),
     .name = "ProjectileEmitterComponent"},

    {.flag = PROJECTILE_COMPONENT_BIT,
     .size = sizeof(ProjectileComponent),
     .alignment = sizeof(ProjectileComponent),
     .name = "ProjectileComponent"},
    {.flag = HEALTH_COMPONENT_BIT,
     .size = sizeof(HealthComponent),
     .alignment = sizeof(HealthComponent),
     .name = "HealthComponent"}};

#endif // COMPONENTS_H
