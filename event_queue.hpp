#ifndef event_queue_hpp
#define event_queue_hpp

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <cstdint>
#include <iostream>
#include <map>
#include <set>

#include "file_descriptor.hpp"

typedef std::pair<uintptr_t, uint16_t> event_identifier;
typedef std::function<void(struct kevent)> event_handler;

struct event_queue {
    event_queue();

    void add_event(int fd, int filter, event_handler handler);
    void delete_event(int fd, int filter);
    void trigger_user_event_handler(uintptr_t);

    void work();

    void stop() { finished = true; }
private:
    int fd;
    std::map<event_identifier, event_handler> events;
    std::set<event_identifier> deleted_events;
    bool finished = false;
};

#endif /* event_queue_hpp */
