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
} HitDetectionSystem;

HitDetectionSystem *hit_detection_system_create(Services *services);

void hit_detection_system_ray_cast_query(HitDetectionSystem *system,
                                         Ray3f *ray);

#endif
