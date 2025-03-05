#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "core/systembase.h"

struct AnimationSystem {
  struct SystemBase base;
};

struct AnimationResource {
  AssetId texture_id;
  uint8_t tile_width;
  uint8_t tile_height;
};

struct AnimationResources {
  struct AnimationResource *resources;
  size_t count;
};

struct AnimationSystem *animation_system_create(struct Services *services);

void animation_system_prepare(struct AnimationSystem *sys,
                              struct AnimationResources *resources);

#endif
