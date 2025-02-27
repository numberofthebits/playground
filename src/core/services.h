#ifndef SERVICES_H
#define SERVICES_H

#include <core/assetstore.h>
#include <core/ecs.h>
#include <core/eventbus.h>

struct Services {
  struct Assets *assets;
  struct EventBus *event_bus;
  struct Registry_t *registry;
};

// Assets* service_get_assets(Services* services);
// void service_set_assets(Services* services, Assets* assets);

// void service_set_event_bus(Services* services, EventBus* event_bus);
// EventBus* service_get_event_bus(Services* services);

// void service_set_registry(Services* services, Registry_t* registry);
// Registry_t* service_get_registry(Services* services);

#endif
