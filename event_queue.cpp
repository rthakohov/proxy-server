#include "event_queue.hpp"

event_queue::event_queue() {
    fd = kqueue();
    if (fd == -1) {
        //TODO: throw error
    }


}

void event_queue::add_event(int fd, int filter, event_handler handler) {
    struct kevent event;
    bzero(&event, sizeof(event));
    int flags = filter == EVFILT_USER ? EV_CLEAR : 0;
    EV_SET(&event, fd, filter, EV_ADD | flags, 0, 0, 0);

    int status = kevent(this -> fd, &event, 1, 0, 0, 0);
    if (status) {
        //TODO: throw error
    }

    auto new_event = std::make_pair(std::make_pair(fd, filter), handler);
    events.erase(new_event.first);
    events.insert(new_event);

    //std::cout << "socket " << fd << " signed up for event " << filter << std::endl;
}

void event_queue::delete_event(int fd, int filter) {
    struct kevent event;
    bzero(&event, sizeof(event));
    EV_SET(&event, fd, filter, EV_DELETE, 0, 0, 0);
    int status = kevent(this -> fd, &event, 1, 0, 0, 0);
    if (status) {
        //TODO: throw error
    }
    deleted_events.insert(std::make_pair(fd, filter));
    //std::cout << "socket " << fd << " cancelled subscription for event " << filter << std::endl;
}

void event_queue::trigger_user_event_handler(uintptr_t ident) {
    struct kevent event;
    EV_SET(&event, ident, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    int fail = kevent(fd, &event, 1, NULL, 0, NULL);
    if (fail) {
        //TODO: throw error
        perror("kevent(USER_EVENT)");
    }
}

void event_queue::work() {
    while (!finished) {
        struct kevent event_list[10];

        int result = kevent(fd, 0, 0, event_list, 10, 0);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                //TODO: throw error
                return;
            }
        }

        for (size_t i = 0; i < result; i++) {
            if (deleted_events.find(std::make_pair(event_list[i].ident, event_list[i].filter)) != deleted_events.end()) {
                continue;
            }
            auto event_it = events.find(std::make_pair(event_list[i].ident, event_list[i].filter));
            if (event_it != events.end()) {
                if (event_it -> second != 0) {
                    //std::cout << "processing event on socket " << event_list[i].ident << std::endl;
                    event_it -> second(event_list[i]);
                }
            }
        }

        deleted_events.clear();
    }
}
