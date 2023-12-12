#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <iostream>

using MiliSeconds = uint64_t;

class Clock
{
public:
    Clock()
        :start_(std::chrono::high_resolution_clock::now()){}

    MiliSeconds GetElapsed()
    {
        return static_cast<MiliSeconds>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_).count());
    }
private:
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};
template<typename T>
class Timer
{
public:
    constexpr Timer()
    :start_(std::chrono::high_resolution_clock::now()), time_out(nullptr){}
    
    constexpr Timer(uint64_t* ptr)
    :start_(std::chrono::high_resolution_clock::now()), time_out(ptr) {}

    constexpr ~Timer()
    {
        const std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
        if(time_out)
        {
            *time_out = static_cast<uint64_t>(std::chrono::duration_cast<T>(end-start_).count());
        }
        else
        {
            std::cout << "Timer lasted: " <<  std::chrono::duration_cast<T>(end-start_).count() << std::endl;
        }
    }
private:
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    uint64_t* time_out;
};

#endif // #ifndef TIMER_HPP