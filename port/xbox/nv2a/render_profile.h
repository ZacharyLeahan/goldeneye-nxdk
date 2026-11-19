#pragma once
#include <stdint.h>

// One renderer thread writes these cumulative counters. Sample at frame end,
// using differences so stage transitions and the CSV write stay outside the window.
struct XboxRenderProfile {
    uint64_t render, submit, acquire, drain, present, uploads;
    uint64_t draws, triangles, bytes;
};
extern XboxRenderProfile xboxRenderProfile;
extern bool xboxProfileEnabled;

// The stock Xbox TSC runs at a fixed rate. Scale deltas by wall time over the
// same sample to report durations without baking in a particular clock rate.
static inline uint64_t xboxRenderClock()
{
    return __builtin_ia32_rdtsc();
}
struct XboxProfileScope {
    uint64_t &total;
    uint64_t start;
    explicit XboxProfileScope(uint64_t &counter)
        : total(counter), start(xboxProfileEnabled ? xboxRenderClock() : 0) {}
    ~XboxProfileScope() { if (start) total += xboxRenderClock() - start; }
};
