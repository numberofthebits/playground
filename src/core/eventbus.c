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

void event_bus_subscribe(struct EventBus* bus, int system_id, int event_id, EventCallback callback) {
    if(bus->num_subscribers >= MAX_SUBSCRIBERS) {
        LOG_ERROR("Failed to subscribe: Exceeds max subscribers %d", MAX_SUBSCRIBERS);
        return;
    }

    struct Subscriber sub;
    sub.callback = callback;
    sub.system_id = system_id;
    sub.event_id = event_id;
    
    bus->subscribers[bus->num_subscribers++] = sub;
}

void event_bus_publish(struct EventBus* bus, int system_id, struct Event event) {
    for (int i = 0; i < bus->num_subscribers; ++i) {
        if (event.id == bus->subscribers[i].event_id) {
            bus->subscribers[i].callback(system_id, &event);
        }
    }
}
