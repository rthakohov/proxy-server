#ifndef dns_resolver_hpp
#define dns_resolver_hpp

#include <iostream>
#include <deque>
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

typedef std::function<void(sockaddr, bool)> callback_t;

struct dns_request {
    dns_request(std::string const& host_name, callback_t callback);
    std::string host_name;
    callback_t callback;

    bool canceled;
};

struct dns_resolver {
    dns_resolver();
    dns_resolver(size_t threads);
    ~dns_resolver();

    void resolve(std::string const& host_name, callback_t callback);
    void stop() { finished = true; }

private:

    //TODO: pointers?
    std::deque<dns_request> queue;
    std::vector<std::thread> threads;
    bool finished = false;

    std::mutex main_mutex;
    std::condition_variable condition;

private:
    void work();

    //helper functions
    sockaddr process_request(dns_request const& req);
};

#endif /* dns_resolver_hpp */
