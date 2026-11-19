#pragma once
#include <kinetra/runtime/engine.h>

namespace kinetra {

/// What a session asks its host to do after an advance.
///
/// A request is state, not an event: the host reads it after every advance and acts on
/// it once. `menu` leaves the session for the host's menu (run_session has no menu and
/// ends the run); `exit` ends the program.
enum class SessionRequest { none, menu, exit };

/// Application-owned behavior. The host owns one session at a time on the runtime thread.
class Session {
  public:
    virtual ~Session() = default;
    /// Advances by `elapsed` seconds with this frame's input. The camera is the host's.
    virtual void advance(double elapsed, const Input &input, Camera2D &camera) = 0;
    /// Draws the current state; called only when request() is `none` after advance().
    virtual void render(Runtime &runtime) = 0;
    /// Window title for the running session: the application's name only. State belongs in
    /// the window itself, not in the title. Hosts may refresh the title less often than
    /// every frame.
    virtual std::string title() = 0;
    /// Read after every advance and before render. Defaults to `none`.
    virtual SessionRequest request() const { return SessionRequest::none; }
    /// Whether Escape closes the window while this session runs. Sessions with their own
    /// exit action return false.
    virtual bool close_on_escape() const { return true; }
};

/// Runs a directly launched session until the window closes, the session requests `menu`
/// or `exit`, or `frame_limit` frames were presented. A request ends the run without
/// rendering that frame. Zero means no frame limit; returns the frames presented.
/// Throws std::invalid_argument for a negative frame limit.
int run_session(Runtime &runtime, Session &session, int frame_limit = 0);
} // namespace kinetra
