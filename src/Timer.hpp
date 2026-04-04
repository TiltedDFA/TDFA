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
    U64 GetTimeAllowance() const
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
        const auto now = std::chrono::steady_clock::now();
        base_ms_ = GetTimeAllowance();
        hard_end_ = now + std::chrono::milliseconds(base_ms_);
        soft_end_ = now + std::chrono::milliseconds(base_ms_ / 2);
        start_ = now;
    }
    bool OutOfTime() const { return std::chrono::steady_clock::now() > hard_end_; }
    bool SoftTimeUp() const { return std::chrono::steady_clock::now() > soft_end_; }

    // Extend soft limit (e.g. when best move changes) up to hard limit
    void ExtendSoftTime(double factor)
    {
        const auto new_soft = start_ + std::chrono::milliseconds(
            static_cast<U64>(base_ms_ * std::min(factor, 1.0)));
        if (new_soft > soft_end_ && new_soft < hard_end_)
            soft_end_ = new_soft;
    }

private:
    U64 our_time_{};
    U64 our_increment_{};
    U64 base_ms_{};
    std::chrono::time_point<std::chrono::steady_clock> start_;
    std::chrono::time_point<std::chrono::steady_clock> hard_end_;
    std::chrono::time_point<std::chrono::steady_clock> soft_end_;
};
#endif // #ifndef TIMER_HPP