#include "platform_utils.h"

#include <dirent.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unordered_map>
#include <algorithm>

namespace Platform {

namespace {

constexpr char const *kProcDirectory = "/proc/";
constexpr char const *kCmdlineFilename = "/cmdline";
constexpr char const *kCpuinfoFilename = "/cpuinfo";
constexpr char const *kStatusFilename = "/status";
constexpr char const *kStatFilename = "/stat";
constexpr char const *kStatPath = "/proc/stat";
constexpr char const *kUptimePath = "/proc/uptime";
constexpr char const *kMeminfoPath = "/proc/meminfo";
constexpr char const *kVersionPath = "/proc/version";
constexpr char const *kOSPath = "/etc/os-release";
constexpr char const *kPasswordPath = "/etc/passwd";

bool StartsWith(std::string_view view, std::string_view subview) {
    return view.substr(0, subview.size()) == subview;
}

template <typename NumT, typename ConvT>
std::unordered_map<std::string_view, NumT> FindNumbers(std::istream &is, std::unordered_set<std::string_view> const &keys, ConvT str_to_num) {
    std::unordered_map<std::string_view, NumT> res;
    std::string line;
    while (std::getline(is, line)) {
        for (const auto key : keys) {
            if (StartsWith(line, key)) {
                char const *str = line.c_str() + key.size();
                NumT const val = str_to_num(str, nullptr, 10);
                res.emplace(key, val);
                break;
            }
        }
        if (res.size() == keys.size()) {
            break;
        }
    }
    return res;
}

template <typename NumT, typename ConvT>
NumT FindNumber(std::istream &is, std::string_view key, ConvT str_to_num) {
    return FindNumbers<NumT>(is, {key}, str_to_num)[key];
}

std::string ProcPath(int pid, char const *fname) {
    std::string path(kProcDirectory);
    path += std::to_string(pid);
    path += fname;
    return path;
}

std::string User(int pid) {
    std::ifstream fs(ProcPath(pid, kStatusFilename));
    const auto uid = FindNumber<int>(fs, "Uid:", strtol);
    const auto uid_str = ":x:" + std::to_string(uid);
    std::ifstream passwd_fs(kPasswordPath);
    std::string line;
    while (std::getline(passwd_fs, line)) {
        if (const auto pos = line.find(uid_str); pos != std::string::npos) {
            return line.substr(0, pos);
        }
    }
    return std::to_string(uid);
}

std::string Command(int pid) {
    std::ifstream fs(ProcPath(pid, kCmdlineFilename));
    std::string res;
    std::getline(fs, res);
    return res;
}

unsigned long long CpuTicks(int pid) {
    std::ifstream fs(ProcPath(pid, kStatFilename));
    std::string line;
    std::getline(fs, line);
    int space_count = 0;
    auto *c = line.c_str();
    auto *end = c + line.size();
    for (; c != end; ++c) {
        if (*c == ' ') {
            ++space_count;
            if (space_count == 13) {
                break;
            }
        }
    }
    char *pend;
    unsigned long long const utime = strtoull(c, &pend, 10);
    unsigned long long const stime = strtoull(pend, nullptr, 10);
    return utime + stime;
}

unsigned long long RamkB(int pid) {
    std::ifstream fs(ProcPath(pid, kStatusFilename));
    return FindNumber<unsigned long long>(fs, "VmSize:", strtoull);
}

unsigned long long StartTime(int pid) {
    std::ifstream fs(ProcPath(pid, kStatFilename));
    std::string line;
    std::getline(fs, line);
    int space_count = 0;
    for (auto *c = line.c_str(), *end = line.c_str() + line.size(); c != end; ++c) {
        if (*c == ' ') {
            ++space_count;
            if (space_count == 21) {
                return strtoull(c + 1, nullptr, 10);
            }
        }
    }
    return 0;
}

} // end namespace

std::string OperatingSystem() {
    constexpr char const *key = "PRETTY_NAME=";

    std::ifstream fs(kOSPath);
    std::string line;
    while (std::getline(fs, line)) {
        if (StartsWith(line, key)) {
            std::string_view val(line.c_str() + strlen(key));
            if (val.size() >= 2) { // exclude quotes
                val.remove_prefix(1);
                val.remove_suffix(1);
            }
            return std::string(val);
        }
    }
    return std::string();
}

std::string Kernel() {
    std::ifstream fs(kVersionPath);
    std::string os, version, kernel;
    fs >> os >> version >> kernel;
    return kernel;
}

unsigned long UpTime() {
    std::ifstream fs(kUptimePath);
    unsigned long secs = 0;
    fs >> secs;
    return secs;
}

RamUtil MemoryUtilization() {
    constexpr char const *total = "MemTotal:";
    constexpr char const *free = "MemFree:";
    constexpr char const *buffers = "Buffers:";
    constexpr char const *cached = "Cached:";
    constexpr char const *shmem = "Shmem:";
    constexpr char const *reclaimable = "SReclaimable:";

    std::ifstream fs(kMeminfoPath);
    auto map = FindNumbers<unsigned long long>(fs, {total, free, buffers, cached, shmem, reclaimable}, strtoull);

    RamUtil res;
    res.total = map[total];
    res.used = res.total - (map[free] + map[buffers] + map[cached] + map[reclaimable] - map[shmem]);
    return res;
}

ProcCounts ProcessCounts() {
    constexpr char const *total = "processes";
    constexpr char const *running = "procs_running";

    std::ifstream fs(kStatPath);
    auto map = FindNumbers<int>(fs, {total, running}, strtol);

    ProcCounts res;
    res.total = map[total];
    res.running = map[running];
    return res;
}

std::vector<CpuUtil> CpuUtilization() {
    std::ifstream fs(kStatPath);
    std::vector<CpuUtil> res;
    std::string line;
    while (std::getline(fs, line) && StartsWith(line, "cpu")) {
        std::istringstream ss(line);
        std::string tmp;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        if (ss >> tmp >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
            CpuUtil util;
            util.idle_ticks = idle + iowait;
            util.total_ticks = util.idle_ticks + user + nice + system + irq + softirq + steal;
            res.push_back(util);
        }
    }
    return res;
}

std::unordered_set<int> Pids() {
    std::unordered_set<int> res;
    auto *directory = opendir(kProcDirectory);
    while (auto *file = readdir(directory)) {
        if (file->d_type == DT_DIR) {
            if (auto *name = file->d_name; std::all_of(name, name + strlen(name), isdigit)) {
                int const pid = strtol(name, nullptr, 10);
                res.insert(pid);
            }
        }
    }
    closedir(directory);
    return res;
}

ProcInfo ProcessInfo(int pid) {
    return ProcInfo {
        User(pid),
        Command(pid),
        StartTime(pid),
        RamkB(pid),
        CpuTicks(pid),
    };
}

} // end namespace Platform
