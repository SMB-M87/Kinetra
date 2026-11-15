#pragma once
#include "engine.h"

namespace kinetra {
/// Application-owned behavior. The host owns one session at a time on the runtime thread.
class Session {
  public:
    virtual ~Session() = default;
    virtual void advance(double elapsed, const Input &input, Camera2D &camera) = 0;
    virtual void render(Runtime &runtime) = 0;
    virtual std::string title() = 0;
};

/// Run a directly launched session; returns frames presented. Zero means no frame limit.
int run_session(Runtime &runtime, Session &session, int frame_limit = 0);
} // namespace kinetra
