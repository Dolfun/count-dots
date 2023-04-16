#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <iostream>

class Debug {
public:
    Debug() {
        id = nr_instances;
        ++nr_instances;
        std::cout << "constructor: " << id << '\n';
    }

    Debug(const Debug& other) {
        std::cout << "copy constructor: " << id << '\n';
    }

    Debug& operator=(const Debug& other) {
        std::cout << "copy assignment operator: " << id << '\n';
        return *this;
    }

    Debug(Debug&& other) {
        std::cout << "move constructor: " << id << '\n';
    }

    Debug& operator=(Debug&& other) {
        std::cout << "move assignment operator: " << id << '\n';
        return *this;
    }

    ~Debug() {
        std::cout << "destructor: " << id << '\n';
    }

private:
    static std::size_t nr_instances;
    std::size_t id;
};

std::size_t Debug::nr_instances = 0;

#endif