#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <memory>

class Process {
public:
    explicit Process(int pid);

    int Pid() const;
    std::string const &User() const;
    std::string const &Command() const;
    float CpuUtilization() const;
    unsigned long Ram() const;
    unsigned long UpTime() const;

    void Update(unsigned long sys_uptime, unsigned long long total_ticks, size_t cpu_count);

private:
    int pid_;
    std::string user_;
    std::string cmd_;
    unsigned long long starttime_;
    unsigned long uptime_;
    unsigned long ram_mb_;
    float cur_cpu_util_;
    unsigned long long total_cpu_util_;
    unsigned long long total_ticks_;
};

#endif
