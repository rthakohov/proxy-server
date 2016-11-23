#include <iostream>
#include <thread>

#include "proxy_server.hpp"
#include "event_queue.hpp"

int main(int argc, const char * argv[]) {
    event_queue queue;
    dns_resolver resolver;
    proxy_server server(2539, queue, resolver);
    queue.work();
    return 0;
}
