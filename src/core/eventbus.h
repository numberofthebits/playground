#ifndef EVENT_BUS_H
#define EVENT_BUS_H

struct Event {
    int type;
    void* event_data;
};

typedef void(*EventCallback)(struct Event* e);

struct EventBus {
    size_t subscribers;
};

void event_bus_subscribe(int system_id, int event_id, );

#endif
