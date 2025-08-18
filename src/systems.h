#ifndef SYSTEMS_H
#define SYSTEMS_H

#include <stddef.h>

typedef enum {
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
  DAMAGE_SYSTEM_BIT = (1U << 9),
  HIT_DETECTION_SYSTEM_BIT = (1U << 10),
  TEXT_SYSTEM_BIT = (1U << 11),
  SYSTEM_BIT_ITER_END = (1U << 12),

  INVALID_SYSTEM_BIT = (1U << 30)
} SystemBit;

#include "animation_system.h"
#include "camera_movement_system.h"
#include "collision_system.h"
#include "damage_system.h"
#include "hit_detection_system.h"
#include "input_system.h"
#include "movement_system.h"
#include "player_system.h"
#include "projectile_emitter_system.h"
#include "render_system.h"
#include "text_system.h"
#include "time_system.h"

#endif
