#ifndef CS_COMPS_TIMER_HPP
#define CS_COMPS_TIMER_HPP

#include <chrono>

namespace cscomps {
namespace util {

// Simple stopwatch-style timer
struct Timer {
    using clock = std::chrono::high_resolution_clock;
    Timer() : start(clock::now()) {}
    void reset() { start = clock::now(); }
    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(clock::now() - start).count();
    }

 private:
    clock::time_point start;
};

// RAII scoped timer that writes elapsed ms into a double reference on destruction
struct ScopedTimer {
    using clock = std::chrono::high_resolution_clock;
    explicit ScopedTimer(double &out_ms) : out(out_ms), t0(clock::now()) {}
    ~ScopedTimer() { out = std::chrono::duration<double, std::milli>(clock::now() - t0).count(); }

 private:
    double &out;
    clock::time_point t0;
};

} // namespace util
} // namespace cscomps

#endif // CS_COMPS_TIMER_HPP
