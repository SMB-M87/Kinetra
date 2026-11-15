#include <chrono>
#include <kinetra/session.h>
#include <stdexcept>

namespace kinetra {
int run_session(Runtime &runtime, Session &session, int frame_limit) {
    if (frame_limit < 0)
        throw std::invalid_argument("Negative frame limit");
    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    int frames = 0;
    while (runtime.poll(session.close_on_escape())) {
        const auto now = Clock::now();
        session.advance(std::chrono::duration<double>(now - previous).count(), runtime.input(),
                        runtime.camera);
        if (session.exit_requested())
            break;
        previous = now;
        session.render(runtime);
        runtime.title(session.title());
        if (++frames == frame_limit)
            break;
    }
    return frames;
}
} // namespace kinetra
