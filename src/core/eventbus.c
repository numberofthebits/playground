#include "eventbus.h"

#include "log.h"

#include <stddef.h>
#include <string.h>

#define EVENT_BUS_DEFERRED_EVENTS_BYTES_MAX 1024 * 1024

void event_bus_init(struct EventBus *bus) {
  LOG_INFO("Initializing event bus with max subscribers %d",
           EVENT_BUS_SUBSCRIBER_COUNT_MAX);

  bus->num_subscribers = 0;
  bus->subscribers = ArenaAlloc(&global_static_allocator,
                                EVENT_BUS_SUBSCRIBER_COUNT_MAX, Subscriber);

  bus->deferred_events_allocator = arena_subarena_create(
      &global_static_allocator, EVENT_BUS_DEFERRED_EVENTS_BYTES_MAX);
  bus->num_deferred_events = 0;
}

void event_bus_reset(struct EventBus *bus) { bus->num_subscribers = 0; }

void event_bus_subscribe(struct EventBus *bus, struct SystemBase *system,
                         int event_id, EventCallback callback) {
  if (bus->num_subscribers >= EVENT_BUS_SUBSCRIBER_COUNT_MAX) {
    LOG_ERROR("Failed to subscribe: Exceeds max subscribers %d",
              EVENT_BUS_SUBSCRIBER_COUNT_MAX);
    return;
  }

  struct Subscriber subscription;
  subscription.callback = callback;
  subscription.system = system;
  subscription.event_id = event_id;

  bus->subscribers[bus->num_subscribers++] = subscription;
}

void event_bus_emit(struct EventBus *bus, struct Event *event) {
  for (size_t i = 0; i < bus->num_subscribers; ++i) {
    struct Subscriber *subscriber = &bus->subscribers[i];
    if (event->id == subscriber->event_id) {
      subscriber->callback(subscriber->system, *event);
    }
  }
}

void event_bus_defer(struct EventBus *bus, struct Event *event) {
  void *event_data_copy =
      arena_subarena_alloc(&bus->deferred_events_allocator,
                           event->event_data_size, alignof(max_align_t));

  memcpy(event_data_copy, event->event_data, event->event_data_size);

  // Note: Rewrite the Event::event_data ptr to point to our copy
  event->event_data = event_data_copy;

  bus->deferred_events[bus->num_deferred_events++] = *event;
}

void event_bus_process_deferred(struct EventBus *bus) {

  size_t num_deferred_events = bus->num_deferred_events;
  size_t num_subscribers = bus->num_subscribers;

  for (size_t i = 0; i < num_deferred_events; ++i) {

    struct Event *event = &bus->deferred_events[i];

    for (size_t j = 0; j < num_subscribers; ++j) {
      struct Subscriber *subscriber = &bus->subscribers[j];

      if (event->id == subscriber->event_id) {
        subscriber->callback(subscriber->system, *event);
      }
    }
  }

  bus->num_deferred_events = 0;
  arena_subarena_dealloc_all(&bus->deferred_events_allocator);
}
