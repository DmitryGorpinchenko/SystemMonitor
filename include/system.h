#ifndef SYSTEM_H
#define SYSTEM_H

#include "process.h"
#include "processor.h"

#include <string>
#include <vector>
#include <algorithm>

class System {
public:
    System();

    std::string const &OperatingSystem() const;
    std::string const &Kernel() const;

    unsigned long UpTime() const;
    float MemoryUtilization() const;
    int TotalProcesses() const;
    int RunningProcesses() const;

    std::vector<Processor> const &Cpus() const;
    std::vector<Process> const &Processes() const;

    template <typename Cmp>
    void OrderProcesses(Cmp cmp, bool invert = false) {
        if (!invert) {
            std::stable_sort(processes_.begin(), processes_.end(), cmp);
        } else {
            std::stable_sort(processes_.rbegin(), processes_.rend(), cmp);
        }
    }

    void Update();

private:
    void UpdateCpus();
    void UpdateProcsList();

    std::string os_ver_;
    std::string kernel_ver_;

    int total_procs_;
    int running_procs_;
    float ram_util_;
    unsigned long uptime_;

    std::vector<Processor> cpus_;
    std::vector<Process> processes_;
};

#endif
