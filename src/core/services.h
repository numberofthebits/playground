#ifndef SERVICES_H
#define SERVICES_H

#include "assetstore.h"
#include "ecs.h"
#include "eventbus.h"
#include "worker.h"

typedef struct {
  struct Assets *assets;
  struct EventBus *event_bus;
  struct Registry_t *registry;
  struct WorkQueue *work_queue;
} Services;

#endif
