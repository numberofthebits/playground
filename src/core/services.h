#ifndef SERVICES_H
#define SERVICES_H

#include "assetstore.h"
#include "ecs.h"
#include "eventbus.h"

// struct Assets;
// struct EventBus;
// struct Registry_t;

typedef struct {
  struct Assets *assets;
  struct EventBus *event_bus;
  struct Registry_t *registry;
} Services;

#endif
