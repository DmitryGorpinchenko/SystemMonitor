#ifndef DEADLINE_H
#define DEADLINE_H

#include <chrono>

template <typename DurationT>
class Deadline {
public:
    Deadline(DurationT deadline)
        : start_(std::chrono::steady_clock::now())
        , deadline_(deadline)
    {}

    DurationT Remaining() const {
        const auto elapsed = std::chrono::steady_clock::now() - start_;
        return (elapsed < deadline_) ? std::chrono::duration_cast<DurationT>(deadline_ - elapsed) : DurationT(0);
    }

    bool Expired() const {
        return Remaining() == DurationT(0);
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> const start_;
    DurationT const deadline_;
};

#endif
