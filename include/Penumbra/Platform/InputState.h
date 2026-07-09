#pragma once

#include "Penumbra/Geometry.h"

#include <string>
#include <vector>

namespace Penumbra::Platform {

// Penumbra's own small keycode vocabulary. Only covers what Penumbra's own widgets
// and documented use cases need — TextInput's editing/navigation/clipboard keys, plus
// Backspace/Escape for consumers implementing zoom-out-style interactions. Anything not
// mapped comes through as Key::Unknown rather than growing this to match SDL's keyspace.
enum class Key {
    Unknown,
    Left, Right, Up, Down,
    Home, End, Backspace, Delete, Escape, Enter, Tab,
    A, C, V, X, // the letters TextInput's Ctrl-shortcuts need; extend as widgets need more
};

struct Modifiers {
    bool Shift{false};
    bool Ctrl{false};
    bool Alt{false};
    bool Super{false};
};

// A snapshot of all OS input for a single frame, built by PlatformWindow.
struct InputState {
    Point MousePosition{};                            // logical pixels
    bool  MouseButtonDown[3]{};                        // left, middle, right — current
    bool  MouseButtonPressedThisFrame[3]{};
    bool  MouseButtonReleasedThisFrame[3]{};
    float MouseWheelDelta{0.0f};                       // accumulated this frame

    std::string       TextInputThisFrame;              // from SDL text-input events
    std::vector<Key>  KeysPressedThisFrame;            // arrows, backspace, delete, etc.
    Modifiers         ModifierState{};

    // Frame timing carried alongside input, so per-frame widget updates (e.g.
    // animations) can advance without changing the WidgetBase interface.
    float DeltaTimeSeconds{0.0f};
};

} // namespace Penumbra::Platform
