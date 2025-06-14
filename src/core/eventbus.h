#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#define MAX_SUBSCRIBERS 1024

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
