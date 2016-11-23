#include "echo_server.hpp"

void nothing(int sgn) {}

proxy_server::proxy_server(int port, event_queue& queue, dns_resolver& resolver) : queue(queue), server(port, queue), resolver(resolver) {

    std::cout << "echo-server running on port " << port << std::endl;

    auto on_client_connected = [&, this](struct kevent event) {
        if (event.ident == server.getfd()) {
            proxy_connection* connection = new proxy_connection(std::move(tcp_socket(server, queue)), queue, *this);
            std::cout << "before init of socket " << connection -> get_client_socket().getfd() << std::endl;
            connection -> get_client_socket().init();
            std::cout << "client " << connection -> get_client_socket().getfd() << " connected\n";
            this -> connections.insert(std::make_pair(connection, std::unique_ptr<proxy_connection>(connection)));
        }
    };

    server.set_on_accept(on_client_connected);

    queue.add_event(SIGINT, EVFILT_SIGNAL, [this](struct kevent event) {
        std::cout << "signal handled\n";
        this -> stop(event.ident);
    });
}

proxy_connection::proxy_connection(tcp_socket&& client, event_queue& queue, proxy_server& proxy) :
    client(std::move(client)), server(std::move(tcp_socket(queue))), queue(queue), proxy(proxy) {
        this -> client.set_on_read([this](std::string s, bool eof) { this -> message_from_client(s, eof); });
    queue.add_event(this -> client.getfd(), EVFILT_USER, [this](struct kevent event) {
        this -> on_host_resolved();
    });
}

void proxy_server::stop(int sgn) {
    for (auto it = connections.begin(); it != connections.end(); it++) {
        proxy_connection::unregister(it -> first -> get_client_socket());
        proxy_connection::unregister(it -> first -> get_server_socket());
    }

    connections.clear();
}


 void proxy_connection::unregister(tcp_socket& socket) {
    socket.unsign();
    close(socket.getfd());
}

void proxy_connection::message_from_client(std::string s, bool eof) {
    if (eof) {
        auto it = proxy.connections.find(this);
        unregister(it -> first -> get_client_socket());
        unregister(it -> first -> get_server_socket());
        proxy.connections.erase(it);
        return;
    }

    if (request) {
        request -> add_part(s);
    } else {
        request.reset(new struct request(s));
    }

    switch (request -> get_state()) {
        case BAD: {
            write_to_client("HTTP/1.1 400 Bad Request\r\n\r\n");
            auto it = proxy.connections.find(this);
            unregister(it -> first -> get_client_socket());
            unregister(it -> first -> get_server_socket());
            //std::cout << "client " << fd << " disconnected\n";
            proxy.connections.erase(it);
            return;
        }
        case FULL_BODY: {
            std::cout << "push to resolve: " << request -> get_host() << " " << request -> get_URI() << std::endl;
            proxy.resolver.resolve(request -> get_host(), [this](struct sockaddr addr, bool success) {
                if (success) {
                    this -> request_addr = addr;
                    queue.trigger_user_event_handler(client.getfd());
                } else {
                    std::cout << "failed to resolve host\n";
                }
            });
            break;
        }

        default:
            break;
    }

}

void proxy_connection::on_host_resolved() {
    std::cout << "host resolved: " << request -> get_host() << std::endl;
    //connecto to server
    connect_to_server();
    //send request to server
    //std::cout << "sending to server: " << request_text << std::endl;
    server.send(request_text.c_str());
}

void proxy_connection::connect_to_server() {
    //save request data
    this -> method = request -> get_method();
    this -> text = request -> get_text();
    this -> uri = request -> get_URI();
    this -> request_text = request -> get_request_text();

    if (server.getfd() != -1) {
        if (request -> get_host() == host) {
            std::cout << "keep-alive for " << host << std::endl;
            request.release();
            return;
        } else {
            server.unsign();
            close(server.getfd());
        }
    }
    this -> host = request -> get_host();
    request.release();

    server = tcp_socket(request_addr, queue);
    server.init();
    server.set_on_read([this](std::string s, bool flag) { this -> message_from_server(s, flag); });
    std::cout << "connected to " << host << std::endl;
}

void proxy_connection::message_from_server(std::string s, bool eof) {
    //TODO: fix this
    client.send(s);
}

void proxy_connection::set_client_callback(callback_t on_read) {
    client.set_on_read(on_read);
}

void proxy_connection::set_server_callback(callback_t on_read) {
    server.set_on_read(on_read);
}

void proxy_connection::write_to_client(std::string s) {
    client.send(s);
}

void proxy_connection::write_to_server(std::string s) {
    server.send(s);
}

void proxy_connection::set_server(tcp_socket server) {
    this -> server = std::move(server);
}
