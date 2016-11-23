#ifndef file_descriptor_hpp
#define file_descriptor_hpp

#include <unistd.h>

struct file_descriptor {
    file_descriptor() : fd(-1) {}
    file_descriptor(int _fd) : fd(_fd) {}
    file_descriptor(file_descriptor const& other) : fd(other.fd) {}

    file_descriptor& operator=(file_descriptor const& rhs);

    int getfd() const;

    void close() const;
private:
    int fd;
};


#endif /* file_descriptor_hpp */
