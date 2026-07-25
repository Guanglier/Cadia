#pragma once
#include <chrono>
#include <iostream>
#include <string>

class ScopedTimer {
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
public:
    ScopedTimer(const std::string& name) : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
        std::cout << "[TIMER] " << m_name << " : " << duration << " µs (" << (duration / 1000.0) << " ms)" << std::endl;
    }
};

