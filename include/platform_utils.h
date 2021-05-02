#ifndef SYSTEM_PARSER_H
#define SYSTEM_PARSER_H

#include <string>
#include <unordered_set>
#include <vector>

namespace Platform {

// System
std::string OperatingSystem();
std::string Kernel();
unsigned long UpTime();

struct RamUtil {
    unsigned long long total = 0;
    unsigned long long used = 0;
};
RamUtil MemoryUtilization();

struct ProcCounts {
    int total = 0;
    int running = 0;
};
ProcCounts ProcessCounts();

// CPU
struct CpuUtil {
    unsigned long long total_ticks = 0;
    unsigned long long idle_ticks = 0;
};
std::vector<CpuUtil> CpuUtilization();

// Processes
std::unordered_set<int> Pids();

struct ProcInfo {
    std::string user;
    std::string command;
    unsigned long long starttime = 0;
    unsigned long long ram_kb = 0;
    unsigned long long cpu_ticks = 0;
};
ProcInfo ProcessInfo(int pid);

} // end namespace Platform

#endif
