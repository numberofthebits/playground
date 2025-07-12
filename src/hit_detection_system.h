#ifndef HIT_DETECTION_SYSTEM_H
#define HIT_DETECTION_SYSTEM_H

#include "components.h"
#include "core/allocators.h"
#include "core/ecs.h"
#include "core/systembase.h"
#include "core/worker.h"
#include "events.h"
#include "systems.h"

typedef struct {
  struct SystemBase base;
  Ray3f *rays;
  size_t num_rays;
  Vec2u16 framebuffer_size;
} HitDetectionSystem;

HitDetectionSystem *hit_detection_system_create(Services *services);

void hit_detection_system_handle_cursor_moved(struct SystemBase *base, Event e);

#endif
