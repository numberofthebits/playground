#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "core/componentbase.h"
#include "core/math.h"
#include "core/os.h"
#include "core/types.h"

#include <stdalign.h>

typedef enum ComponentBit {
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
  HEALTH_COMPONENT_BIT = (1U << 10),
  MESH_COMPONENT_BIT = (1U << 11),
  TEXT_COMPONENT_BIT = (1U << 12)
} ComponentBit;

typedef struct TransformComponent {
  Vec3f pos;
  float scale;    // DON'T support shearing.
  float rotation; // Single axis. You pick one for now.
} TransformComponent;

/* typedef struct { */
/*   AssetId material_id; */
/*   AssetId pipeline_id; */
/*   Vec2u8 texture_atlas_index; */
/*   Vec2u8 texture_atlas_size; */
/* } RenderComponentMapTile; */

/* typedef struct RenderComponentUnit { */
/*   AssetId material_id; */
/*   AssetId pipeline_id; */
/*   Vec2u8 texture_atlas_index; */
/*   Vec2u8 texture_atlas_size; */
/*   uint8_t animation_frame; */
/* } RenderComponentUnit; */

/* typedef enum RenderComponentType { */
/*   RenderComponentTypeMapTile, */
/*   RenderComponentTypeUnit */
/* } RenderComponentType; */

#define RENDER_COMPONENT_RENDER_LAYER_TERRAIN 0
#define RENDER_COMPONENT_RENDER_LAYER_GROUND 1
#define RENDER_COMPONENT_RENDER_LAYER_AIR 2

typedef struct {

  // Normalized texture coordinate modifier
  // Remember. In GL, positive Y is up. In image
  // space, positive Y is down.
  Vec2u8 texture_atlas_index;
  Vec2u8 texture_atlas_size;
  AssetId material_id;
  AssetId pipeline_id;
  /* AssetId mesh_id; */
  uint8_t render_layer;
} RenderComponent;

typedef struct PhysicsComponent {
  // TODO: Replace this with two scalars:
  //       angle and velocity.
  Vec2f velocity;
} PhysicsComponent;

typedef struct AnimationComponent {
  uint8_t frames_per_animation_frame;
  uint8_t num_animation_frames;
  uint8_t num_frames_width;
  uint8_t num_frames_height;
  uint8_t is_playing;
} AnimationComponent;

typedef struct CollisionComponent {
  // axis aligned bounding rect
  // When paired with TransformComponent, pos should
  // act as an offset, so generally leave pos at 0 in this case
  float width;
  float height;
} CollisionComponent;

typedef struct InputComponent {
  uint8_t dummy;
} InputComponent;

typedef struct TimeComponent {
  TimeT created;
  TimeT expires;
} TimeComponent;

typedef struct CameraMovementComponent {
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

typedef struct {
  Mesh mesh;
} MeshComponent;

#define TEXT_COMPONENT_FLAG_SCREEN_SPACE 0x1

typedef struct TextComponent {
  const char *text;
  uint32_t len;
  Vec4u8 color;
  float scale_factor;
  int flags;
} TextComponent;

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
     .alignment = alignof(HealthComponent),
     .name = "HealthComponent"},

    {.flag = MESH_COMPONENT_BIT,
     .size = sizeof(MeshComponent),
     .alignment = alignof(MeshComponent),
     .name = "MeshComponent"},

    {.flag = TEXT_COMPONENT_BIT,
     .size = sizeof(TextComponent),
     .alignment = alignof(TextComponent),
     .name = "TextComponent"}};

#endif // COMPONENTS_H
