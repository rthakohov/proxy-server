#ifndef echo_server_hpp
#define echo_server_hpp

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include <signal.h>

#include "event_queue.hpp"
#include "socket.hpp"
#include "new_http_handler.hpp"
#include "dns_resolver.hpp"

struct proxy_connection;

struct proxy_server {
    proxy_server(int port, event_queue& queue, dns_resolver& resolver);
    ~proxy_server() { stop(0); };

    std::map<proxy_connection*, std::unique_ptr<proxy_connection> > connections;

    dns_resolver& resolver;
private:
    server_socket server;
    event_queue& queue;

    void stop(int sgn);
};

struct proxy_connection {
    typedef std::function<void (std::string, bool)> callback_t;

    proxy_connection(tcp_socket&& client, event_queue& queue, proxy_server& proxy);

    void set_client_callback(callback_t on_read);
    void set_server_callback(callback_t on_write);

    void set_server(tcp_socket server);

    void write_to_client(std::string s);
    void write_to_server(std::string s);

    void message_from_client(std::string, bool);
    void message_from_server(std::string, bool);

    void on_host_resolved();

    static void unregister(tcp_socket& socket);

    tcp_socket& get_client_socket() { return client; }
    tcp_socket& get_server_socket() { return server; }

private:
    tcp_socket client;
    tcp_socket server;

    event_queue& queue;

    std::unique_ptr<response> response;
    std::unique_ptr<request> request;

    std::shared_ptr<dns_request> request_ptr;

    proxy_server& proxy;

    void connect_to_server();

    //the following fields describe the current task:
    std::string host;
    struct sockaddr request_addr;
    std::string uri;
    std::string method;
    std::string text;
    std::string request_text;
};

#endif /* echo_server_hpp */
