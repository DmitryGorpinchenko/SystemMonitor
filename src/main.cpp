#include "ncurses_display.h"
#include "system.h"

#include <thread>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

struct Opts {
    int interval_ds;

    Opts(int argc, char **argv)
        : interval_ds(15)
    {
        int opt;
        while ((opt = getopt(argc, argv, "d:")) != -1) {
            switch (opt) {
            case 'd':
                if (int const val = strtol(optarg, nullptr, 10); val > 0) {
                    interval_ds = val;
                }
                break;
            }
        }
    }
};

int main(int argc, char **argv) {
    Opts opts(argc, argv);
    System system;
    NCurses::Display disp(system, NCurses::Display::deciseconds(opts.interval_ds));
    return 0;
}
