#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kinetra {

/// Fixed update clock with a bounded backlog. Long stalls are deliberately dropped
/// to keep input responsive; interpolation() is the remaining fraction of a tick.
class FixedClock {
  public:
    explicit FixedClock(double step = 1.0 / 120, int max_steps = 16) : step_(step), max_steps_(max_steps) {
        if (!std::isfinite(step) || step <= 0 || max_steps <= 0 || !std::isfinite(step * max_steps))
            throw std::invalid_argument("Invalid fixed clock settings");
    }
    int advance(double elapsed) {
        if (!std::isfinite(elapsed) || elapsed < 0)
            throw std::invalid_argument("Elapsed time must be finite and nonnegative");
        accumulator_ = std::min(accumulator_ + elapsed, step_ * max_steps_);
        const int steps = std::min(int((accumulator_ + step_ * 1e-9) / step_), max_steps_);
        accumulator_ = std::max(0.0, accumulator_ - steps * step_);
        return steps;
    }
    double step() const { return step_; }
    double interpolation() const { return accumulator_ / step_; }

  private:
    double step_, accumulator_{};
    int max_steps_;
};

} // namespace kinetra
