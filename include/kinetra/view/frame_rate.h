#pragma once
// The FPS row of a debug menu and the frame pacing it asks of the Runtime. Headless: an
// application's logic layer owns the setting and builds its row with the other rows; the
// presentation hands it the Runtime once per frame.

#include <kinetra/view/debug_menu.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace kinetra {

/// How a FrameRateSetting asks the Runtime to space frames.
enum class FrameRateMode {
    /// Presenting waits for the display (Runtime::vsync).
    vsync,
    /// The frame limiter aims for FrameRateSetting::limit() frames per second.
    limit,
    /// Frames come as fast as they are drawn.
    uncapped,
};

/// A session's FPS setting: one debug-menu row, and the Runtime calls that carry it out.
///
/// The rows of a debug menu are built and run in a session's advance(), often by headless
/// logic, where the Runtime is out of reach, so the row only changes this object; apply(),
/// called once per drawn frame, passes a change on. The runtime outlives sessions and owns the pacing, so the
/// first apply() reads what it does instead: a session opened from a launcher keeps the choice of the one
/// before it, and a host that switched the limit off (a capture tool) keeps it off. Not
/// copyable, because the row refers to it.
class FrameRateSetting {
  public:
    /// Lowest and highest frame limit, as Runtime::frame_limit accepts them.
    static constexpr std::uint64_t slowest = 1, fastest = 1000;
    /// Frames per second a tick budget assumes when frames are uncapped: a frame that takes
    /// longer than this to simulate would make the window feel slow.
    static constexpr double uncapped_rate = 60;

    FrameRateSetting() = default;
    FrameRateSetting(const FrameRateSetting &) = delete;
    FrameRateSetting &operator=(const FrameRateSetting &) = delete;

    /// The FPS row: its value is VSYNC, the limit, or UNCAPPED. A click on the label moves to
    /// the next of VSYNC, the limit and UNCAPPED; `[-]` and `[+]` halve and double the limit,
    /// and a number typed over the row sets it (see debug_rate), each switching to the limit.
    DebugEntry entry(int depth = 1) {
        auto row = debug_rate(
            "FPS", [this] { return limit_; },
            [this](std::uint64_t value) {
                limit_ = value;
                mode_ = FrameRateMode::limit;
            },
            slowest, fastest, depth);
        row.value = [this] {
            switch (mode_) {
            case FrameRateMode::vsync:
                return std::string("VSYNC");
            case FrameRateMode::uncapped:
                return std::string("UNCAPPED");
            default:
                return std::to_string(limit_);
            }
        };
        row.action = [this] {
            mode_ = mode_ == FrameRateMode::vsync   ? FrameRateMode::limit
                    : mode_ == FrameRateMode::limit ? FrameRateMode::uncapped
                                                    : FrameRateMode::vsync;
        };
        return row;
    }
    /// What the row asks for.
    FrameRateMode mode() const { return mode_; }
    /// Frames per second of the limit, also while another mode is chosen.
    std::uint64_t limit() const { return limit_; }

    /// Call once per drawn frame with the kinetra::Runtime, before drawing. The first call
    /// reads the runtime's pacing; later calls pass a changed mode or limit on. Where the
    /// platform refuses VSync, or refuses to stop it, the setting follows what the runtime
    /// does. A template only so that this header needs no window: `Runtime` is
    /// kinetra::Runtime, or a stand-in with its pacing calls in a test.
    template <class Runtime> void apply(Runtime &runtime) {
        using Pacing = decltype(runtime.pacing());
        if (!synchronized_) {
            synchronized_ = true;
            const auto pacing = runtime.pacing();
            mode_ = pacing == Pacing::vsync     ? FrameRateMode::vsync
                    : pacing == Pacing::limited ? FrameRateMode::limit
                                                : FrameRateMode::uncapped;
            limit_ = std::clamp<std::uint64_t>(
                static_cast<std::uint64_t>(std::lround(1 / runtime.frame_interval())), slowest, fastest);
        } else if (mode_ != applied_mode_ || limit_ != applied_limit_) {
            runtime.frame_limit(static_cast<double>(limit_));
            runtime.limit_frames(mode_ != FrameRateMode::uncapped);
            if (runtime.vsync(mode_ == FrameRateMode::vsync) != (mode_ == FrameRateMode::vsync))
                mode_ = runtime.pacing() == Pacing::vsync ? FrameRateMode::vsync : FrameRateMode::limit;
        }
        applied_mode_ = mode_;
        applied_limit_ = limit_;
        interval_ = mode_ == FrameRateMode::uncapped ? 1 / uncapped_rate : runtime.frame_interval();
    }
    /// Seconds between frames at the last apply(): the time a frame may spend, and so the
    /// share of it a tick budget can use. 1/uncapped_rate before the first apply() and while
    /// frames are uncapped.
    double interval() const { return interval_; }

  private:
    FrameRateMode mode_{FrameRateMode::vsync}, applied_mode_{FrameRateMode::vsync};
    std::uint64_t limit_{60}, applied_limit_{60};
    double interval_{1 / uncapped_rate};
    bool synchronized_{};
};

} // namespace kinetra
