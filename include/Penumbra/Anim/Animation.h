#pragma once

#include "Penumbra/Render/Color.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

// Small, header-only animation toolkit. Two models, because they answer different
// questions:
//   * AnimatedColor / Approach — framerate-independent easing toward a moving target
//     with no fixed endpoint (hover/press state transitions).
//   * Tween + easing curves — a one-shot 0->1 timeline of a known duration, for pops
//     and fades (e.g. a modal open/close — designed-for, not yet wired up).
namespace Penumbra::Anim {

// Exponential smoothing toward Target. Framerate-independent: the same TimeConstant
// gives the same motion at any frame rate. TimeConstant <= 0 snaps instantly.
inline float Approach(float Current, float Target, float DeltaSeconds, float TimeConstantSeconds) {
    if (TimeConstantSeconds <= 0.0f) {
        return Target;
    }
    const float Factor = 1.0f - std::exp(-DeltaSeconds / TimeConstantSeconds);
    return Current + (Target - Current) * Factor;
}

inline Render::Color LerpColor(Render::Color A, Render::Color B, float T) {
    T = std::clamp(T, 0.0f, 1.0f);
    const auto Mix = [&](std::uint8_t a, std::uint8_t b) {
        return static_cast<std::uint8_t>(std::lround(a + (b - a) * T));
    };
    return {Mix(A.R, B.R), Mix(A.G, B.G), Mix(A.B, B.B), Mix(A.A, B.A)};
}

// A colour that eases toward whatever target it is handed each frame. Channels are
// kept as floats so small per-frame steps don't stall on Uint8 rounding.
struct AnimatedColor {
    float R{0.0f};
    float G{0.0f};
    float B{0.0f};
    float A{0.0f};
    bool  Initialised{false};

    void SnapTo(Render::Color Target) {
        R = Target.R;
        G = Target.G;
        B = Target.B;
        A = Target.A;
        Initialised = true;
    }

    void Animate(Render::Color Target, float DeltaSeconds, float TimeConstantSeconds) {
        if (!Initialised) {
            SnapTo(Target); // first frame: no fade-in from nothing
            return;
        }
        R = Approach(R, Target.R, DeltaSeconds, TimeConstantSeconds);
        G = Approach(G, Target.G, DeltaSeconds, TimeConstantSeconds);
        B = Approach(B, Target.B, DeltaSeconds, TimeConstantSeconds);
        A = Approach(A, Target.A, DeltaSeconds, TimeConstantSeconds);
    }

    Render::Color Value() const {
        const auto Clamp = [](float V) {
            return static_cast<std::uint8_t>(std::lround(std::clamp(V, 0.0f, 255.0f)));
        };
        return {Clamp(R), Clamp(G), Clamp(B), Clamp(A)};
    }
};

// --- one-shot timelines (ready for modal pops; t in [0,1]) ---

inline float EaseOutCubic(float T) {
    const float U = 1.0f - T;
    return 1.0f - U * U * U;
}

// Overshoots past 1.0 then settles — reads as a "pop".
inline float EaseOutBack(float T) {
    constexpr float C1 = 1.70158f;
    constexpr float C3 = C1 + 1.0f;
    const float U = T - 1.0f;
    return 1.0f + C3 * U * U * U + C1 * U * U;
}

// A 0->1 timeline over Duration seconds. Drive Update() each frame; read Progress().
struct Tween {
    float Elapsed{0.0f};
    float Duration{0.0f};
    bool  Playing{false};

    void Start(float DurationSeconds) {
        Duration = DurationSeconds;
        Elapsed = 0.0f;
        Playing = true;
    }

    void Update(float DeltaSeconds) {
        if (!Playing) {
            return;
        }
        Elapsed += DeltaSeconds;
        if (Elapsed >= Duration) {
            Elapsed = Duration;
            Playing = false;
        }
    }

    float Progress() const {
        return (Duration > 0.0f) ? std::clamp(Elapsed / Duration, 0.0f, 1.0f) : 1.0f;
    }
};

} // namespace Penumbra::Anim
