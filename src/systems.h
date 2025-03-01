#ifndef SYSTEM_H
#define SYSTEM_H

#include <stddef.h>

enum SystemBit {
  SYSTEM_BIT_ITER_BEGIN = (1U << 0),

  MOVEMENT_SYSTEM_BIT = (1U << 0),
  RENDER_SYSTEM_BIT = (1U << 1),
  COLLISION_SYSTEM_BIT = (1U << 2),
  ANIMATION_SYSTEM_BIT = (1U << 3),
  INPUT_SYSTEM_BIT = (1U << 4),
  TIME_SYSTEM_BIT = (1U << 5),
  PLAYER_SYSTEM_BIT = (1U << 6),
  CAMERA_MOVEMENT_SYSTEM_BIT = (1U << 7),
  PROJECTILE_EMITTER_SYSTEM_BIT = (1U << 8),

  SYSTEM_BIT_ITER_END = (1U << 8),

  INVALID_SYSTEM_BIT = (1U << 30)
};
typedef enum SystemBit SystemBit;

// const System system_table[] = {
//     {
//         .name = "MovementSystem",
//         .flag = MOVEMENT_SYSTEM_BIT,
//         .evaluation_order = 0,
//     },
//     {
//         .name = "RenderSystem",
//         .flag = RENDER_SYSTEM_BIT,
//         .evaluation_order = 0,
//     },
//     {
//         .name = "RenderSystem",
//         .flag = RENDER_SYSTEM_BIT,
//         .evaluation_order = 0,
//     },
//     {.name = "AnimationSystem",
//      .flag = ANIMATION_SYSTEM_BIT,
//      .evaluation_order = 0},
//     {
//         .name = "InputSystem",
//         .flag = INPUT_SYSTEM_BIT,
//         .evaluation_order = 0,
//     },
//     {.name = "TimeSystem", .flag = TIME_SYSTEM_BIT, .evaluation_order = 0},
//     {.name = "PlayerSystem", .flag = PLAYER_SYSTEM_BIT, .evaluation_order =
//     0},
//     {.name = "CameraMovementSystem",
//      .flag = CAMERA_MOVEMENT_SYSTEM_BIT,
//      .evaluation_order = 0},
//     {.name = "ProjectileEmitterSystem",
//      .flag = PROJECTILE_EMITTER_SYSTEM_BIT,
//      .evaluation_order = 0}};

#endif
