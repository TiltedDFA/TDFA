#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <iostream>

template<typename T>
class Timer
{
public:
    constexpr Timer()
    :start_(std::chrono::high_resolution_clock::now()), time_out(nullptr){}
    
    constexpr Timer(U64* ptr)
    :start_(std::chrono::high_resolution_clock::now()), time_out(ptr) {}

    constexpr ~Timer()
    {
        const std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
        if(time_out)
        {
            *time_out = U64(std::chrono::duration_cast<T>(end-start_).count());
        }
        else
        {
            std::cout << "Timer lasted: " <<  std::chrono::duration_cast<T>(end-start_).count() << std::endl;
        }
    }
private:
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    U64* time_out;
};
class TimeManager
{
private:
    //miliseconds
    U64 GetTimeAllowance()const
    {
        return U64(our_time_ / 20 + our_increment_ / 2);
    }
public:
    void SetOptions(U64 time, U64 increment)
    {
        our_time_ = time;
        our_increment_ = increment;
    }
    void StartTiming()
    {
        end_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(GetTimeAllowance());
    }
    bool OutOfTime()const {return std::chrono::steady_clock::now() > end_;}

private:
    U64 our_time_;
    U64 our_increment_;
    std::chrono::time_point<std::chrono::steady_clock> end_;
};
#endif // #ifndef TIMER_HPP