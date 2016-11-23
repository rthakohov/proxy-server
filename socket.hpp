#ifndef socket_hpp
#define socket_hpp

#include "socket.hpp"
#include "event_queue.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/event.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <iostream>
#include <exception>
#include <cstring>
#include <string>
#include <deque>
#include <cstdio>
#include <set>
#include <cstring>

//TODO: constructors and operators
//TODO: destruction

struct server_socket {
    typedef std::function<void (struct kevent)> callback_t;

    server_socket(int port, event_queue& queue);

    server_socket& operator=(server_socket const&);

    void set_on_accept(callback_t);

    int getfd() const { return fd; }

private:
    int fd;
    event_queue& queue;
};


struct tcp_socket {
    typedef std::function<void (std::string, bool)> callback_t;

    //Initialization
    tcp_socket(tcp_socket&& other);
    tcp_socket(event_queue& queue) : queue(queue), fd(-1) {}
    tcp_socket(struct sockaddr addr, event_queue& queue);
    tcp_socket(server_socket const& server, event_queue& queue);

    tcp_socket& operator=(tcp_socket&& rhs);

    //Methods
    void set_on_read(callback_t on_read);
    void send(std::string);
    /**
    * Switches the socket into non-blocking mode and sets calllbacks
    */
    void init();

    int getfd() { return fd; }

    void unsign() const;

private:
    event_queue& queue;
    void signup();
    std::string read();
    size_t write(std::string);
    std::deque<std::string> msg_queue;
    callback_t on_read;
    void on_write_handler(struct kevent);

    int fd;

    void set_nonblocking();
};




#endif /* socket_hpp */
