#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <iostream>

class Debug {
public:
    Debug() {
        id = _nr_instances;
        ++_nr_instances;
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

    static std::size_t nr_instances() {
        return _nr_instances;
    }

private:
    static std::size_t _nr_instances;
    std::size_t id;
};

std::size_t Debug::_nr_instances = 0;

#endif