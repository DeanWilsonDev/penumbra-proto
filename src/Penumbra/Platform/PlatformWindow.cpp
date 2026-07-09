#include "Penumbra/Platform/PlatformWindow.h"

namespace Penumbra::Platform {

namespace {
// SDL mouse buttons are 1-based (left=1, middle=2, right=3); InputState indexes 0-based.
constexpr Uint32 ButtonMask[3] = {SDL_BUTTON_LMASK, SDL_BUTTON_MMASK, SDL_BUTTON_RMASK};

// Maps SDL's keycode space onto Penumbra's small, deliberately incomplete Key
// vocabulary (spec item 5) — only the keys Penumbra's own widgets and documented use
// cases care about. Anything else comes through as Key::Unknown.
Key MapKey(SDL_Keycode Code) {
    switch (Code) {
    case SDLK_LEFT:      return Key::Left;
    case SDLK_RIGHT:     return Key::Right;
    case SDLK_UP:        return Key::Up;
    case SDLK_DOWN:      return Key::Down;
    case SDLK_HOME:      return Key::Home;
    case SDLK_END:       return Key::End;
    case SDLK_BACKSPACE: return Key::Backspace;
    case SDLK_DELETE:    return Key::Delete;
    case SDLK_ESCAPE:    return Key::Escape;
    case SDLK_RETURN:    return Key::Enter;
    case SDLK_TAB:       return Key::Tab;
    case SDLK_A:         return Key::A;
    case SDLK_C:         return Key::C;
    case SDLK_V:         return Key::V;
    case SDLK_X:         return Key::X;
    default:             return Key::Unknown;
    }
}

Modifiers MapModifiers(SDL_Keymod Mod) {
    return {(Mod & SDL_KMOD_SHIFT) != 0, (Mod & SDL_KMOD_CTRL) != 0,
            (Mod & SDL_KMOD_ALT) != 0, (Mod & SDL_KMOD_GUI) != 0};
}
} // namespace

bool PlatformWindow::Initialise(const char* Title, int LogicalWidth, int LogicalHeight) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }

    const SDL_WindowFlags Flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!SDL_CreateWindowAndRenderer(Title, LogicalWidth, LogicalHeight, Flags,
                                     &Window, &SdlRenderer)) {
        SDL_Quit();
        return false;
    }

    // Sync presentation to the display refresh. Without this the loop renders
    // thousands of uncapped frames per second, pegging the GPU — which shows up as
    // coil whine and needless power draw. One frame per refresh is plenty for a UI.
    SDL_SetRenderVSync(SdlRenderer, 1);

    DpiScaleFactor = SDL_GetWindowDisplayScale(Window);
    if (DpiScaleFactor <= 0.0f) {
        DpiScaleFactor = 1.0f;
    }

    LastFrameTimeNs = SDL_GetTicksNS();
    return true;
}

void PlatformWindow::Shutdown() {
    if (SdlRenderer) {
        SDL_DestroyRenderer(SdlRenderer);
        SdlRenderer = nullptr;
    }
    if (Window) {
        SDL_DestroyWindow(Window);
        Window = nullptr;
    }
    SDL_Quit();
}

bool PlatformWindow::PumpEventsAndBuildInput(InputState& OutInputState) {
    const Uint64 Now = SDL_GetTicksNS();
    float Delta = static_cast<float>(static_cast<double>(Now - LastFrameTimeNs) / 1.0e9);
    LastFrameTimeNs = Now;
    if (Delta > 0.1f) {
        Delta = 0.1f; // clamp pauses (e.g. a stalled frame) so animations don't jump
    }
    OutInputState.DeltaTimeSeconds = Delta;

    OutInputState.TextInputThisFrame.clear();
    OutInputState.KeysPressedThisFrame.clear();
    OutInputState.MouseWheelDelta = 0.0f;

    bool KeepRunning = true;

    SDL_Event Event;
    while (SDL_PollEvent(&Event)) {
        switch (Event.type) {
        case SDL_EVENT_QUIT:
            KeepRunning = false;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            OutInputState.MouseWheelDelta += Event.wheel.y;
            break;
        case SDL_EVENT_TEXT_INPUT:
            OutInputState.TextInputThisFrame += Event.text.text;
            break;
        case SDL_EVENT_KEY_DOWN:
            OutInputState.KeysPressedThisFrame.push_back(MapKey(Event.key.key));
            break;
        default:
            break;
        }
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    const SDL_MouseButtonFlags Buttons = SDL_GetMouseState(&MouseX, &MouseY);
    OutInputState.MousePosition = {MouseX, MouseY};

    for (int Index = 0; Index < 3; ++Index) {
        const bool Down = (Buttons & ButtonMask[Index]) != 0;
        const bool WasDown = PreviousMouseButtonDown[Index];

        OutInputState.MouseButtonDown[Index] = Down;
        OutInputState.MouseButtonPressedThisFrame[Index] = Down && !WasDown;
        OutInputState.MouseButtonReleasedThisFrame[Index] = !Down && WasDown;

        PreviousMouseButtonDown[Index] = Down;
    }

    OutInputState.ModifierState = MapModifiers(SDL_GetModState());

    return KeepRunning;
}

Point PlatformWindow::GetLogicalWindowSize() const {
    int Width = 0;
    int Height = 0;
    if (Window) {
        SDL_GetWindowSize(Window, &Width, &Height);
    }
    return {static_cast<float>(Width), static_cast<float>(Height)};
}

float PlatformWindow::GetDpiScaleFactor() const {
    const float Live = SDL_GetWindowDisplayScale(Window);
    return (Live > 0.0f) ? Live : DpiScaleFactor; // fall back to the cached value if the query fails
}

void PlatformWindow::SetTextInputActive(bool Active) {
    if (!Window) {
        return;
    }
    if (Active) {
        SDL_StartTextInput(Window);
    } else {
        SDL_StopTextInput(Window);
    }
}

SDL_Renderer* PlatformWindow::GetSdlRenderer() const {
    return SdlRenderer;
}

void PlatformWindow::SetTitle(const std::string& Title) {
    SDL_SetWindowTitle(Window, Title.c_str());
}

std::string PlatformWindow::GetLastError() const {
    return SDL_GetError();
}

void PlatformWindow::SetClipboardText(const std::string& Text) {
    SDL_SetClipboardText(Text.c_str());
}

std::string PlatformWindow::GetClipboardText() const {
    if (!SDL_HasClipboardText()) {
        return {};
    }
    char* Raw = SDL_GetClipboardText(); // SDL allocates; we own it
    std::string Result = (Raw != nullptr) ? Raw : "";
    SDL_free(Raw);
    return Result;
}

} // namespace Penumbra::Platform
