#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "platform_utils.h"

class Processor {
public:
    Processor();

    unsigned long long TotalTicks() const;
    float Utilization() const;

    void Update(Platform::CpuUtil const &total_util);

private:
    float cur_util_;
    Platform::CpuUtil total_util_;
};

#endif
