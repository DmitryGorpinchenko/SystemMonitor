#include "process.h"
#include "platform_utils.h"

#include <unistd.h>

Process::Process(int pid)
    : pid_(pid)
    , starttime_(0)
    , uptime_(0)
    , ram_mb_(0)
    , cur_cpu_util_(0.f)
    , total_cpu_util_(0)
    , total_ticks_(0)
{}

int Process::Pid() const { return pid_; }
std::string const &Process::User() const { return user_; }
std::string const &Process::Command() const { return cmd_; }
float Process::CpuUtilization() const { return cur_cpu_util_; }
unsigned long Process::Ram() const { return ram_mb_; }
unsigned long Process::UpTime() const { return uptime_; }

void Process::Update(unsigned long sys_uptime, unsigned long long total_ticks, size_t cpu_count) {
    const auto info = Platform::ProcessInfo(pid_);

    user_ = info.user;
    cmd_ = info.command;
    starttime_ = info.starttime;
    uptime_ = sys_uptime - starttime_ / sysconf(_SC_CLK_TCK);
    ram_mb_ = info.ram_kb / 1000;

    const auto sub = [](auto l, auto r) { return (l > r) ? (l - r) : 0; };
    const auto dused = sub(info.cpu_ticks, total_cpu_util_);
    const auto dtotal = sub(total_ticks, total_ticks_);
    cur_cpu_util_ = cpu_count * static_cast<float>(dused) / dtotal;
    total_cpu_util_ = info.cpu_ticks;
    total_ticks_ = total_ticks;
}

