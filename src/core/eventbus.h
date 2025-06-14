#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#define EVENT_BUS_SUBSCRIBER_COUNT_MAX 1024
#define EVENT_BUS_DEFERRED_EVENT_COUNT_MAX 1024

#include "allocators.h"

// Forward declare as systembase needs to know about
// EventBus impl
struct SystemBase;

// General event type header
// The producer and consumer needs to know the meaning
// of both the type of event, its type. The size size
// of the event data is required for the event bus
// to work with it, without caring about its actual type
typedef struct Event {
  int id;
  void *event_data;
  size_t event_data_size;
} Event;

typedef void (*EventCallback)(struct SystemBase *system, struct Event e);

struct Subscriber {
  EventCallback callback;
  struct SystemBase *system;
  int event_id;
};
typedef struct Subscriber Subscriber;

// TODO: Introduce an allocator for the event bus so we can avoid allocating
//       events on the stack when emitting them. We know when to clear this
//       memory because the events are delivered synchronously.
//       Is the StackAllocator a good fit here?
typedef struct EventBus {
  size_t num_subscribers;
  struct Subscriber *subscribers;
  SubArenaAllocator deferred_events_allocator;
  size_t num_deferred_events;
  struct Event deferred_events[EVENT_BUS_DEFERRED_EVENT_COUNT_MAX];
} EventBus;

void event_bus_init(struct EventBus *bus);

// Clear all subscriptions
void event_bus_reset(struct EventBus *bus);

// Subscribe to events from a system... Is input handling a system?
void event_bus_subscribe(struct EventBus *bus, struct SystemBase *system,
                         int event_id, EventCallback callback);

void event_bus_emit(struct EventBus *bus, struct Event *event);

void event_bus_defer(struct EventBus *bus, struct Event *event);

void event_bus_process_deferred(struct EventBus *bus);

#endif
