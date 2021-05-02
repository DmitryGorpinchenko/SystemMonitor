#include "processor.h"

namespace {

Platform::CpuUtil operator -(Platform::CpuUtil const &lhs, Platform::CpuUtil const &rhs) {
    const auto sub = [](auto l, auto r) { return (l > r) ? (l - r) : 0; };
    Platform::CpuUtil res;
    res.total_ticks = sub(lhs.total_ticks, rhs.total_ticks);
    res.idle_ticks = sub(lhs.idle_ticks, rhs.idle_ticks);
    return res;
}

} // end namespace

Processor::Processor()
    : cur_util_(0.f)
{}

unsigned long long Processor::TotalTicks() const { return total_util_.total_ticks; }
float Processor::Utilization() const { return cur_util_; }

void Processor::Update(Platform::CpuUtil const &total_util) {
    const auto dutil = total_util - total_util_;
    cur_util_ = static_cast<float>(dutil.total_ticks - dutil.idle_ticks) / dutil.total_ticks;
    total_util_ = total_util;
}
