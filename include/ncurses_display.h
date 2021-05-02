#ifndef NCURSES_DISPLAY_H
#define NCURSES_DISPLAY_H

#include "system.h"
#include "process.h"
#include "deadline.h"

#include <curses.h>
#include <vector>
#include <chrono>

namespace NCurses {

class Display {
public:
    using deciseconds = std::chrono::duration<long long, std::deci>;

    explicit Display(System &system, deciseconds interval);
    ~Display();

private:
    void Run();
    bool Iteration();

    void Update();
    void Render();
    void RenderSystem(int &row);
    void RenderProcs(int &row);

    void OrderProcs();
    void Scroll(size_t proc_count, size_t page_size);
    bool Filter(Process const &proc) const;

    void ProcessInput(int c);

    System &system_;
    deciseconds interval_;

    enum class ScrollAction {
        NONE, UP, DOWN, PAGE_UP, PAGE_DOWN, HOME, END,
    } scroll_action_;
    size_t proc_offset_;
    enum class ProcOrderKey : int {
        CPU, RAM, UPTIME,
    } order_key_;
    bool invert_order_;
    bool show_kernel_threads_;

    bool stopped_;
    bool quit_;
    bool update_;
    bool render_;
    WINDOW *window_;
};

} // end namespace NCurses

#endif
