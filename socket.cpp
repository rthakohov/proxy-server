#include "socket.hpp"

server_socket::server_socket(int port, event_queue& queue) : queue(queue) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        //TODO: throw error
        perror("socket()");
        exit(0);
    }

    this -> fd = fd;

    //init the sockaddr struct
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    //make the socket reuse its address
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //bind the listening socket to a port
    int tmp;
    if ((tmp = bind(fd, (sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        perror("failed to bind the socket\n");
    }

    //start listening
    if ((tmp = listen(fd, 10)) < 0) {
        perror("failed to start listening\n");
    }

    //std::cout << "socket " << fd << " bound to port " << port << " and listening" << std::endl;

}

server_socket& server_socket::operator=(server_socket const& other) {
    this -> fd = other.fd;
    this -> queue = queue;
    return *this;
}

void server_socket::set_on_accept(callback_t callback) {
    queue.add_event(fd, EVFILT_READ, callback);
}

tcp_socket::tcp_socket(tcp_socket&& other) : queue(other.queue) {
    std::swap(queue, other.queue);
    std::swap(fd, other.fd);
    std::swap(msg_queue, other.msg_queue);
    std::swap(on_read, other.on_read);
}

tcp_socket& tcp_socket::operator=(tcp_socket&& rhs) {
    //TODO: close(fd)
    std::swap(queue, rhs.queue);
    std::swap(fd, rhs.fd);
    std::swap(msg_queue, rhs.msg_queue);
    std::swap(on_read, rhs.on_read);

    return *this;
}

tcp_socket::tcp_socket(struct sockaddr addr, event_queue& queue) : queue(queue) {
    //create a socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        //TODO: throw error
        perror("socket()");
        exit(0);
    }

    this -> fd = fd;

    std::cout << "socket " << fd << " created" << std::endl;

    //connect
    /*struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &servaddr.sin_addr) <= 0) {
        perror("inet_pton()");
    }*/

    if (connect(fd, &addr /*(struct sockaddr *) &servaddr*/, sizeof(addr)) < 0) {
        perror("connect()");
    }

    //std::cout << "socket " << fd << " connected to address " << address << " at port " << port << std::endl;

}

tcp_socket::tcp_socket(server_socket const& server, event_queue& queue) : queue(queue) {
    int fd = accept(server.getfd(), 0, 0);
    if (fd < 0) {
        //TODO: throw error
        perror("accept()");
        exit(0);
    }

    this -> fd = fd;
}

void tcp_socket::unsign() const {
    if (fd != -1) {
        queue.delete_event(fd, EVFILT_READ);
        queue.delete_event(fd, EVFILT_WRITE);
    }
}

void tcp_socket::init() {
    std::cout << "init socket " << getfd() << std::endl;
    //TODO: should I make connect() non-blocking?
    set_nonblocking();

    //signup for events
    std::function<void (struct kevent)> on_read_handler = [this](struct kevent event) {
        std::string data;
        bool eof_flag = false;
        if (event.flags & EV_EOF) {
            eof_flag = true;
        } else {
            data = read();
            if (data.empty()) {
                eof_flag = true;
            }
        }
        if (!data.empty() || eof_flag) {
            if (on_read != 0) {
                on_read(data, eof_flag);
            }
        }
    };
    std::function<void (struct kevent)> on_write_handler = [this](struct kevent event) {
        this -> on_write_handler(event);
    };
    queue.add_event(fd, EVFILT_READ, on_read_handler);
    queue.add_event(fd, EVFILT_WRITE, on_write_handler);

}

void tcp_socket::on_write_handler(struct kevent event) {
    while (!this -> msg_queue.empty()) {
        std::string cur = msg_queue.front();
        msg_queue.pop_front();

        size_t written = write(cur);
        if (written < cur.size()) {
            cur = cur.substr(written, std::string::npos);
            msg_queue.push_front(cur);
            break;
        }
    }
    queue.delete_event(fd, EVFILT_WRITE);
}

void tcp_socket::set_on_read(callback_t on_read) {
    this -> on_read = on_read;
}

void tcp_socket::send(std::string data) {
    //std::cout << "message added to socket " << getfd() << ": " << message << std::endl;
    msg_queue.push_back(data);
    queue.add_event(fd, EVFILT_WRITE, [this](struct kevent event) { this -> on_write_handler(event); });
}

/**
* Attempts to write a string to to the socket
* Returns the number of written symbols
*/
size_t tcp_socket::write(std::string data) {
    if (data.size() == 0) {
        return 0;
    }
    //std::cout << "writing to client: " << data << std::endl;
    //std::cout << "writing to socket " << fd << std::endl;
    ssize_t count = ::send(fd, data.c_str(), data.size(), 0);
    if (count < 0) {
        if (errno != EAGAIN && errno != ENOTCONN) {
            perror("send()");
            exit(0);
        }
        return 0;
    }

    return count;
}

std::string tcp_socket::read() {
    //std::cout << "reading from " << getfd() << std::endl;

    char buffer[255];
    ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
    if (count < 0) {
        if (errno != EAGAIN) {
            perror("recv\n");
            exit(0);
        }
    }

    std::string s = "";
    for (size_t i = 0; i < count; i++) {
        s += buffer[i];
    }

    return s;
}

void tcp_socket::set_nonblocking() {
    const int set = 1;
    if (setsockopt(getfd(), SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set)) == -1) { // NOSIGPIPE FOR SEND
        //TODO: throw error
        perror("setsockopt()");
        std::cout << getfd() << std::endl;
        exit(0);
    };

    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
