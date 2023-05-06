#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <utility>

class Debug {
public:
    Debug() {
        id = _nr_instances;
        ++_nr_instances;
        std::cout << "constructor: " << id << '\n';
    }

    Debug(const Debug&) {
        std::cout << "copy constructor: " << id << '\n';
    }

    Debug& operator=(const Debug&) {
        std::cout << "copy assignment operator: " << id << '\n';
        return *this;
    }

    Debug(Debug&&) {
        std::cout << "move constructor: " << id << '\n';
    }

    Debug& operator=(Debug&&) {
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

class Profiler {
public:
    template<typename Functor>
    void profile(std::string name, Functor&& f) {
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        total_duration_profiled += duration.count();
        data.emplace_back(name, duration.count());
    }

    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        auto _end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = _end - _start;
        total_duration = duration.count();
        data.emplace_back("other", total_duration - total_duration_profiled);
    }

    void print_results() {
        std::size_t max_length = 0;
        for (auto p : data) max_length = std::max(max_length, p.first.size());
        for (auto [name, duration] : data) {
            std::cout << std::left << std::setw(max_length) << name << " "
                      << std::fixed << std::setprecision(3) << duration << "s  "
                      << std::right << std::setw(5) << std::setprecision(2) << (duration / total_duration * 100.0) << "%\n";
        }
        std::cout << "total duration: " << total_duration << "s\n";
    }
private:
    std::vector<std::pair<std::string, double>> data;
    double total_duration_profiled = 0.0, total_duration = 0.0;

    std::chrono::high_resolution_clock::time_point _start;
};

#endif