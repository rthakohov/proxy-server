#include "dns_resolver.hpp"

dns_resolver::dns_resolver() : dns_resolver::dns_resolver(1) {}

dns_resolver::dns_resolver(size_t threads_count) {
    for (size_t i = 0; i < threads_count; i++) {
        threads.push_back(std::thread(&dns_resolver::work, this));
    }
}

dns_resolver::~dns_resolver() {
    finished = true;
    condition.notify_all();
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join();
    }
}

void dns_resolver::work() {
    //TODO: caching
    while (!finished) {
        std::unique_lock<std::mutex> lk(main_mutex);
        condition.wait(lk, [&] {
            //std::cout << "queue.size() = " << queue.size() << '\n';
            return finished || !queue.empty();
        });

        if (finished) {
            break;
        }

        dns_request current = queue.front();
        queue.pop_front();
        main_mutex.unlock();

        if (current.canceled) {
            continue;
        }

        struct sockaddr resolved;
        bool success = true;
        try {
            resolved = process_request(current);
        } catch (std::runtime_error const& err) {
            std::cout << err.what() << std::endl;
            success = false;
        }
        if (!finished && current.callback != nullptr) {
            //TODO: callback from a different thread is bad
            current.callback(resolved, success);
        }
    }
}

void dns_resolver::resolve(std::string const& host_name, callback_t callback) {
    dns_request req(host_name, callback);
    queue.push_back(req);
    condition.notify_one();
}

dns_request::dns_request(std::string const& host_name, callback_t callback) {
    this -> host_name = host_name;
    this -> callback = callback;
}

sockaddr dns_resolver::process_request(dns_request const& req) {
    //simulate slow connection
    //std::this_thread::sleep_for(std::chrono::seconds(2));

    struct addrinfo hints, *resolved;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    int rv = getaddrinfo(req.host_name.c_str(), "80", &hints, &resolved);
    if (rv) {
        throw std::runtime_error("getaddrinfo()");
    }

    sockaddr res = *(resolved -> ai_addr);
    freeaddrinfo(resolved);
    return res;
}
