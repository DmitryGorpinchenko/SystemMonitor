#include "system.h"
#include "platform_utils.h"

System::System()
    : os_ver_(Platform::OperatingSystem())
    , kernel_ver_(Platform::Kernel())
    , total_procs_(0)
    , running_procs_(0)
    , ram_util_(0.f)
    , uptime_(0)
    , cpus_(std::vector<Processor>(2))
{}

std::string const &System::OperatingSystem() const { return os_ver_; }
std::string const &System::Kernel() const { return kernel_ver_; }
unsigned long System::UpTime() const { return uptime_; }
float System::MemoryUtilization() const { return ram_util_; }
int System::TotalProcesses() const { return total_procs_; }
int System::RunningProcesses() const { return running_procs_; }
std::vector<Processor> const &System::Cpus() const { return cpus_; }
std::vector<Process> const &System::Processes() const { return processes_; }

void System::Update() {
    const auto proc_counts = Platform::ProcessCounts();
    total_procs_ = proc_counts.total;
    running_procs_ = proc_counts.running;

    const auto util = Platform::MemoryUtilization();
    ram_util_ = static_cast<float>(util.used) / util.total;

    uptime_ = Platform::UpTime();
    UpdateCpus();
    UpdateProcsList();
}

void System::UpdateCpus() {
    const auto total_util = Platform::CpuUtilization();
    if (total_util.size() < 2) {
        return;
    }
    cpus_.resize(total_util.size());
    for (size_t i = 0; i < total_util.size(); ++i) {
        cpus_[i].Update(total_util[i]);
    }
}

void System::UpdateProcsList() {
    auto pids = Platform::Pids();

    processes_.erase(
        std::remove_if(processes_.begin(), processes_.end(), [&pids](Process const &p) { return pids.find(p.Pid()) == pids.end(); }),
        processes_.end()
    );
    for (auto const &p : processes_) {
        pids.erase(p.Pid());
    }
    for (int const pid : pids) {
        processes_.push_back(Process(pid));
    }
    for (auto &p : processes_) {
        p.Update(uptime_, cpus_[0].TotalTicks(), cpus_.size() - 1); // cpus_[0] is an aggregate 'cpu'
    }
}
