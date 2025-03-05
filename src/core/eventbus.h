#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#define MAX_SUBSCRIBERS 1024

#include "systembase.h"

struct SystemBase;

// General event type
// The producer and consumber needs to know the meaning
// of both the type of event and its data
struct Event {
  int id;
  void *event_data;
};

typedef void (*EventCallback)(struct SystemBase *system, struct Event e);

struct Subscriber {
  EventCallback callback;
  struct SystemBase *system;
  int event_id;
};
typedef struct Subscriber Subscriber;

struct EventBus {
  size_t num_subscribers;
  struct Subscriber *subscribers;
};

void event_bus_init(struct EventBus *bus);

// Clear all subscriptions
void event_bus_reset(struct EventBus *bus);

// Subscribe to events from a system... Is input handling a system?
void event_bus_subscribe(struct EventBus *bus, struct SystemBase *system,
                         int event_id, EventCallback callback);

void event_bus_emit(struct EventBus *bus, struct Event *event);

#endif
