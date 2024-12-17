#include "eventbus.h"

#include "arena.h"
#include "log.h"

void event_bus_init(struct EventBus* bus) {
    LOG_INFO("Initializing event bus with max subscribers %d", MAX_SUBSCRIBERS);
    
    bus->num_subscribers = 0;
    bus->subscribers = ArenaAlloc(&allocator, MAX_SUBSCRIBERS, Subscriber);
}

void event_bus_reset(struct EventBus* bus) {
    bus->num_subscribers = 0;
}

void event_bus_subscribe(struct EventBus* bus, struct SystemBase* system, int event_id, EventCallback callback) {
    if(bus->num_subscribers >= MAX_SUBSCRIBERS) {
        LOG_ERROR("Failed to subscribe: Exceeds max subscribers %d", MAX_SUBSCRIBERS);
        return;
    }

    struct Subscriber subscription;
    subscription.callback = callback;
    subscription.system = system;
    subscription.event_id = event_id;
    
    bus->subscribers[bus->num_subscribers++] = subscription;
}

void event_bus_emit(struct EventBus* bus, struct Event* event) {
    for (int i = 0; i < bus->num_subscribers; ++i) {
        struct Subscriber* subscriber = &bus->subscribers[i];
        if (event->id == subscriber->event_id) {
            subscriber->callback(subscriber->system, *event);
        }
    }
}
