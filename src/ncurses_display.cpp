#include "ncurses_display.h"
#include "format.h"

#include <string>
#include <algorithm>
#include <limits>

namespace NCurses {

namespace {

std::string ToString(float val, int decimals) {
    const auto str = std::to_string(val);
    const auto pos = str.find('.');
    return str.substr(0, (pos == std::string::npos) ? pos : (pos + decimals + (decimals > 0)));
}

// 50 bars uniformly displayed from 0 - 100 %
// 2% is one bar(|)
std::string ProgressBar(float percent) {
    int const size = 50;
    int const bars = static_cast<int>(percent * size);

    std::string result("0%");
    for (int i = 0; i < size; ++i) {
        result += (i <= bars) ? '|' : ' ';
    }
    result += ' ';
    const auto str = ToString(percent * 100, percent < 1.f);
    if (str.size() < 4) {
        for (size_t i = 0; i < 4 - str.size(); ++i) {
            result += ' ';
        }
    }
    result += str;
    result += "/100%";
    return result;
}

using std::chrono::milliseconds;
int Getch(milliseconds timeout) {
    timeout(timeout.count());
    return getch();
}

} // end namespace

Display::Display(System &system, deciseconds interval)
    : system_(system)
    , interval_(interval)
    , scroll_action_(ScrollAction::NONE)
    , proc_offset_(0)
    , order_key_(ProcOrderKey::CPU)
    , invert_order_(false)
    , show_kernel_threads_(false)
    , stopped_(false)
    , quit_(false)
    , update_(false)
    , render_(true)
{
    initscr();            // start ncurses
    noecho();             // do not print input values
    cbreak();             // terminate ncurses on ctrl + c
    keypad(stdscr, true); // enable special keys
    start_color();        // enable color
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);

    window_ = newwin(0, 0, 0, 0);
    refresh();

    Run();
}

Display::~Display() {
    endwin();
}

void Display::Run() {
    while (Iteration())
        ;
}

bool Display::Iteration() {
    Deadline<milliseconds> deadline(interval_);
    Update();
    do {
        Render();
        milliseconds const timeout = !stopped_ ? deadline.Remaining() : milliseconds(-1);
        ProcessInput(Getch(timeout));
    } while (!quit_ && !update_ && (stopped_ || !deadline.Expired()));
    return !quit_;
}

void Display::Update() {
    system_.Update();
    update_ = false;
    render_ = true;
}

void Display::Render() {
    if (!render_) {
        return;
    }
    werase(window_);

    int row = 0;
    RenderSystem(row);
    RenderProcs(++row);

    wrefresh(window_);
    refresh();
    render_ = false;
}

void Display::RenderSystem(int &row) {
    mvwprintw(window_, ++row, 2, ("OS: " + system_.OperatingSystem()).c_str());
    mvwprintw(window_, ++row, 2, ("Kernel: " + system_.Kernel()).c_str());
    auto const &cpus = system_.Cpus();
    for (size_t i = 1 /*exclude aggregate cpu*/; i < cpus.size(); ++i) {
        std::string caption = "CPU ";
        caption += std::to_string(i);
        caption += ": ";
        mvwprintw(window_, ++row, 2, caption.c_str());
        wattron(window_, COLOR_PAIR(1));
        mvwprintw(window_, row, 10, "");
        wprintw(window_, ProgressBar(cpus[i].Utilization()).c_str());
        wattroff(window_, COLOR_PAIR(1));
    }
    mvwprintw(window_, ++row, 2, "Memory: ");
    wattron(window_, COLOR_PAIR(2));
    mvwprintw(window_, row, 10, "");
    wprintw(window_, ProgressBar(system_.MemoryUtilization()).c_str());
    wattroff(window_, COLOR_PAIR(2));
    mvwprintw(window_, ++row, 2, ("Total Processes: " + std::to_string(system_.TotalProcesses())).c_str());
    mvwprintw(window_, ++row, 2, ("Running Processes: " + std::to_string(system_.RunningProcesses())).c_str());
    mvwprintw(window_, ++row, 2, ("Up Time: " + Format::ElapsedTime(system_.UpTime())).c_str());
}

void Display::RenderProcs(int &row) {
    constexpr int pid_column = 2;
    constexpr int user_column = 9;
    constexpr int cpu_column = 20;
    constexpr int ram_column = 30;
    constexpr int time_column = 40;
    constexpr int command_column = 50;
    wattron(window_, COLOR_PAIR(2));
    mvwprintw(window_, ++row, 0, std::string(window_->_maxx + 1, ' ').c_str());
    mvwprintw(window_, row, pid_column, "PID");
    mvwprintw(window_, row, user_column, "USER");
    mvwprintw(window_, row, cpu_column, "CPU[%%]");
    mvwprintw(window_, row, ram_column, "RAM[MB]");
    mvwprintw(window_, row, time_column, "TIME+");
    mvwprintw(window_, row, command_column, "COMMAND");
    wattroff(window_, COLOR_PAIR(2));
    OrderProcs();
    auto const &procs = system_.Processes();
    size_t const proc_count = std::count_if(procs.begin(), procs.end(), [this](Process const &p) { return Filter(p); });
    size_t const page_size = std::max(0, window_->_maxy - 1 - row);
    Scroll(proc_count, page_size);
    size_t i = 0, offset_count = 0;
    for (; (i < procs.size()) && (offset_count < proc_offset_); offset_count += Filter(procs[i]), ++i)
        ;
    for (; (i < procs.size()) && (row < window_->_maxy - 1); ++i) {
        Process const &proc = procs[i];
        if (!Filter(proc)) {
            continue;
        }
        mvwprintw(window_, ++row, pid_column, std::to_string(proc.Pid()).c_str());
        mvwprintw(window_, row, user_column, proc.User().c_str());
        mvwprintw(window_, row, cpu_column, ToString(proc.CpuUtilization() * 100, 1).c_str());
        mvwprintw(window_, row, ram_column, std::to_string(proc.Ram()).c_str());
        mvwprintw(window_, row, time_column, Format::ElapsedTime(proc.UpTime()).c_str());
        mvwprintw(window_, row, command_column, proc.Command().substr(0, window_->_maxx - command_column).c_str());
    }
    for (; row < window_->_maxy - 1; ++row)
        ;
    wattron(window_, COLOR_PAIR(1));
    mvwprintw(window_, ++row, 0, std::string(window_->_maxx + 1, ' ').c_str());
    wattroff(window_, COLOR_PAIR(1));
}

void Display::OrderProcs() {
    switch (order_key_) {
    case ProcOrderKey::CPU:
        system_.OrderProcesses(
            [](Process const &lhs, Process const &rhs) { return lhs.CpuUtilization() > rhs.CpuUtilization(); },
            invert_order_
        );
        break;
    case ProcOrderKey::RAM:
        system_.OrderProcesses(
            [](Process const &lhs, Process const &rhs) { return lhs.Ram() > rhs.Ram(); },
            invert_order_
        );
        break;
    case ProcOrderKey::UPTIME:
        system_.OrderProcesses(
            [](Process const &lhs, Process const &rhs) { return lhs.UpTime() < rhs.UpTime(); },
            invert_order_
        );
        break;
    }
}

void Display::Scroll(size_t proc_count, size_t page_size) {
    switch (scroll_action_) {
    case ScrollAction::UP:
        if (proc_offset_ > 0) {
            --proc_offset_;
        }
        break;
    case ScrollAction::DOWN:
        ++proc_offset_;
        break;
    case ScrollAction::PAGE_UP:
        if (proc_offset_ > page_size) {
            proc_offset_ -= page_size;
        } else {
            proc_offset_ = 0;
        }
        break;
    case ScrollAction::PAGE_DOWN:
        proc_offset_ += page_size;
        break;
    case ScrollAction::HOME:
        proc_offset_ = 0;
        break;
    case ScrollAction::END:
        if (proc_count > page_size) {
            proc_offset_ = proc_count - page_size;
        } else {
            proc_offset_ = 0;
        }
        break;
    default: break;
    }
    proc_offset_ = std::min(proc_offset_, proc_count - std::min(proc_count, page_size)); // fit
    scroll_action_ = ScrollAction::NONE;
}

bool Display::Filter(Process const &proc) const {
    if (!show_kernel_threads_) {
        return !proc.Command().empty();
    }
    return true;
}

void Display::ProcessInput(int c) {
    switch (c) {
    case 'q':
        quit_ = true;
        break;
    case 's':
        stopped_ = !stopped_;
        update_ = !stopped_;
        break;
    case 'r':
        update_ = true;
        break;
    case KEY_DOWN:
        scroll_action_ = ScrollAction::DOWN;
        render_ = true;
        break;
    case KEY_UP:
        scroll_action_ = ScrollAction::UP;
        render_ = true;
        break;
    case KEY_NPAGE:
        scroll_action_ = ScrollAction::PAGE_DOWN;
        render_ = true;
        break;
    case KEY_PPAGE:
        scroll_action_ = ScrollAction::PAGE_UP;
        render_ = true;
        break;
    case KEY_HOME:
        scroll_action_ = ScrollAction::HOME;
        render_ = true;
        break;
    case KEY_END:
        scroll_action_ = ScrollAction::END;
        render_ = true;
        break;
    case KEY_RESIZE:
        refresh();
        render_ = true;
        break;
    case 'p':
        order_key_ = ProcOrderKey::CPU;
        invert_order_ = false;
        render_ = true;
        update_ = !stopped_;
        break;
    case 'm':
        order_key_ = ProcOrderKey::RAM;
        invert_order_ = false;
        render_ = true;
        update_ = !stopped_;
        break;
    case 't':
        order_key_ = ProcOrderKey::UPTIME;
        invert_order_ = false;
        render_ = true;
        update_ = !stopped_;
        break;
    case 'i':
        invert_order_ = !invert_order_;
        render_ = true;
        break;
    case 'k':
        show_kernel_threads_ = !show_kernel_threads_;
        render_ = true;
        break;
    }
}

} // end namespace NCurses
