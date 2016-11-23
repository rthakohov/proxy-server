#include "file_descriptor.hpp"

file_descriptor& file_descriptor::operator=(file_descriptor const& rhs) {
    this -> fd = rhs.fd;
    return *this;
}

void file_descriptor::close() const {
    if (fd != -1) {
        int result = ::close(fd);
        if (result != 0) {
            //TODO: do something
        }
    }
}

int file_descriptor::getfd() const {
    return fd;
}
