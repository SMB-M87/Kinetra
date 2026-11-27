#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
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

/// A tick rate in ticks per wall-clock second, for a simulation whose tick has a fixed length
/// in simulated time: the rate changes how fast the simulation runs, not what it computes.
///
/// advance() turns elapsed wall-clock seconds into the whole ticks now due, at most a limit
/// the caller chooses, typically from a TickBudget; ticks owed beyond the limit are dropped,
/// so a rate that the processor cannot reach slows the simulation down instead of freezing
/// the window. The time is kept in ticks, not seconds, so a million ticks per second loses
/// no precision to a tiny step. Headless and deterministic: it reads no clock of its own.
class TickClock {
  public:
    /// `rate`: ticks per second. Throws std::invalid_argument unless it is finite and
    /// positive.
    explicit TickClock(double rate) { this->rate(rate); }
    /// Ticks per second.
    double rate() const { return rate_; }
    /// Sets the ticks per second and drops the fraction of a tick accumulated so far, so
    /// interpolation() starts from the last tick again. Throws std::invalid_argument unless
    /// `rate` is finite and positive.
    void rate(double rate) {
        if (!std::isfinite(rate) || rate <= 0)
            throw std::invalid_argument("A tick rate must be finite and positive");
        rate_ = rate;
        owed_ = 0;
    }
    /// Adds `elapsed` seconds and returns how many whole ticks to run now, at most `limit`;
    /// the rest of a longer backlog is dropped, except the fraction of the next tick. Throws
    /// std::invalid_argument for negative or non-finite time.
    std::uint64_t advance(double elapsed, std::uint64_t limit) {
        if (!std::isfinite(elapsed) || elapsed < 0)
            throw std::invalid_argument("Elapsed time must be finite and nonnegative");
        owed_ += elapsed * rate_;
        // A sum of exact frame times, such as 1/120 s at 120 ticks per second, can land a hair
        // below a whole tick; the tolerance runs that tick now, as FixedClock does.
        const double whole = std::floor(owed_ + 1e-9);
        // Whatever the limit cuts off is dropped: only the fraction of the next tick stays.
        owed_ = std::max(0.0, owed_ - whole);
        return static_cast<std::uint64_t>(std::min(whole, double(limit)));
    }
    /// Fraction of the next tick already accumulated, in [0, 1).
    double interpolation() const { return std::min(owed_, 1.0 - 1e-12); }

  private:
    double rate_{}, owed_{};
};

/// How many ticks fit in a share of wall-clock time, from what earlier ticks cost.
///
/// The caller times its ticks and record()s them; ticks() divides a budget by the average
/// cost per tick. The average follows changes in a few frames, so a simulation that grows
/// busier gets fewer ticks per frame and the frame rate holds. Before the first record it
/// allows `first` ticks, a guess that the first measurement corrects. Headless: the caller
/// supplies the time.
class TickBudget {
  public:
    /// `first`: ticks allowed before any measurement. Throws std::invalid_argument for zero.
    explicit TickBudget(std::uint64_t first = 64) : first_(first) {
        if (first == 0)
            throw std::invalid_argument("A first tick budget must be positive");
    }
    /// Remembers that `ticks` ticks took `seconds`. Ignores zero ticks. Throws
    /// std::invalid_argument for negative or non-finite time.
    void record(std::uint64_t ticks, double seconds) {
        if (!std::isfinite(seconds) || seconds < 0)
            throw std::invalid_argument("Tick time must be finite and nonnegative");
        if (ticks == 0)
            return;
        const double cost = seconds / double(ticks);
        // Weighted by a quarter, so one slow frame (a page fault, a replan) moves the
        // estimate a little and a lasting change moves it within a few frames.
        cost_ = cost_ > 0 ? cost_ + (cost - cost_) * 0.25 : cost;
    }
    /// The ticks that fit in `seconds`, at least one, so the simulation never stops. Throws
    /// std::invalid_argument for negative or non-finite time.
    std::uint64_t ticks(double seconds) const {
        if (!std::isfinite(seconds) || seconds < 0)
            throw std::invalid_argument("A tick budget must be finite and nonnegative");
        if (cost_ <= 0)
            return first_;
        // Capped far above any rate a frame can run, so the conversion cannot overflow.
        return std::max<std::uint64_t>(1, static_cast<std::uint64_t>(std::min(seconds / cost_, 1e12)));
    }
    /// The average seconds per tick; zero before the first record.
    double cost() const { return cost_; }

  private:
    std::uint64_t first_;
    double cost_{};
};

} // namespace kinetra
