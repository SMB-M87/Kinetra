#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kinetra {

/// Fixed update clock with a bounded backlog. Long stalls are deliberately dropped
/// to keep input responsive; interpolation() is the remaining fraction of a tick.
class FixedClock {
  public:
    /// `step`: seconds per tick. `max_steps`: the largest number of ticks one advance() may
    /// return; a longer backlog is dropped. Throws std::invalid_argument unless both are
    /// positive and finite.
    explicit FixedClock(double step = 1.0 / 120, int max_steps = 16) : step_(step), max_steps_(max_steps) {
        if (!std::isfinite(step) || step <= 0 || max_steps <= 0 || !std::isfinite(step * max_steps))
            throw std::invalid_argument("Invalid fixed clock settings");
    }
    /// Adds `elapsed` seconds and returns how many whole ticks to run now, at most
    /// `max_steps`. Throws std::invalid_argument for negative or nonfinite time.
    int advance(double elapsed) {
        if (!std::isfinite(elapsed) || elapsed < 0)
            throw std::invalid_argument("Elapsed time must be finite and nonnegative");
        accumulator_ = std::min(accumulator_ + elapsed, step_ * max_steps_);
        const int steps = std::min(int((accumulator_ + step_ * 1e-9) / step_), max_steps_);
        accumulator_ = std::max(0.0, accumulator_ - steps * step_);
        return steps;
    }
    /// Seconds per tick.
    double step() const { return step_; }
    /// Fraction of the next tick already accumulated, in [0, 1).
    double interpolation() const { return accumulator_ / step_; }

  private:
    double step_, accumulator_{};
    int max_steps_;
};

/// Measures how often something happens per second, over fixed windows of elapsed time.
///
/// Frame, update and decision counters use it for statistics overlays: count() the events,
/// advance() by the frame's elapsed seconds, and read rate(). The rate changes once per
/// window, so the displayed number stays readable. Throws std::invalid_argument for a
/// window that is not finite and positive, or for negative or non-finite elapsed time.
class RateCounter {
  public:
    /// `window`: seconds per measurement window.
    explicit RateCounter(double window = 0.5) : window_(window) {
        if (!std::isfinite(window) || window <= 0)
            throw std::invalid_argument("Invalid rate window");
    }
    /// Counts `events` events in the current window.
    void count(int events = 1) { events_ += events; }
    /// Adds elapsed seconds; when the window is full, publishes events per second and
    /// starts a new window.
    void advance(double elapsed) {
        if (!std::isfinite(elapsed) || elapsed < 0)
            throw std::invalid_argument("Elapsed time must be finite and nonnegative");
        elapsed_ += elapsed;
        if (elapsed_ >= window_) {
            rate_ = events_ / elapsed_;
            elapsed_ = 0;
            events_ = 0;
        }
    }
    /// Events per second over the last complete window; zero before the first one.
    double rate() const { return rate_; }

  private:
    double window_, elapsed_{}, rate_{};
    int events_{};
};

} // namespace kinetra
