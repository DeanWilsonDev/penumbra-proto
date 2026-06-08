#include "DemoResolvers.h"
#include "DemoTheme.h"

#include "Penumbra/Platform/PlatformWindow.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Render/SdlTtfFontBackend.h"
#include "Penumbra/Widgets/Box.h"
#include "Penumbra/Widgets/Button.h"

#include <cstdio>
#include <memory>
#include <string>

namespace {

// Logical window size for the demo. A demo concern, never a Penumbra concern.
constexpr int WindowLogicalWidth  = 960;
constexpr int WindowLogicalHeight = 640;

constexpr const char* FontFileName = "JetBrainsMonoNerdFontMono-Regular.ttf";

using namespace Penumbra::Widgets;

} // namespace

int main() {
    Demo::Theme Theme;

    Penumbra::Platform::PlatformWindow Window;
    if (!Window.Initialise("Penumbra Demo", WindowLogicalWidth, WindowLogicalHeight)) {
        std::fprintf(stderr, "Failed to initialise platform window: %s\n", SDL_GetError());
        return 1;
    }

    std::printf("DPI scale factor: %.3f\n", Window.GetDpiScaleFactor());
    std::printf("Hover and click the top button; the bottom button is disabled.\n");
    std::fflush(stdout);

    {
        Penumbra::Render::SdlTtfFontBackend FontBackend;
        const std::string FontPath = std::string(DEMO_ASSET_DIR) + "/" + FontFileName;
        [[maybe_unused]] const Penumbra::Render::FontHandle BodyFont =
            FontBackend.LoadFont(FontPath.c_str(), Theme.FontSizeBody, Window.GetDpiScaleFactor());

        Penumbra::Render::Renderer Renderer;
        if (!Renderer.Initialise(Window.GetSdlRenderer(), Window.GetDpiScaleFactor(), &FontBackend)) {
            std::fprintf(stderr, "Failed to initialise renderer\n");
            Window.Shutdown();
            return 1;
        }

        // A panel stacking an interactive button above a disabled one.
        auto Root = std::make_unique<Box>();
        Root->Style    = Demo::ResolvePanelStyle(Theme);
        Root->Layout   = LayoutMode::VerticalStack;
        Root->ChildGap = Theme.SpacingMedium;

        int ClickCount = 0;

        auto Primary = std::make_unique<Button>();
        Primary->ApplyStyle(Demo::ResolvePrimaryButtonStyle(Theme));
        Primary->OnClicked = [&ClickCount]() {
            ++ClickCount;
            std::printf("Primary button clicked (count = %d)\n", ClickCount);
            std::fflush(stdout);
        };

        auto Disabled = std::make_unique<Button>();
        Disabled->ApplyStyle(Demo::ResolvePrimaryButtonStyle(Theme));
        Disabled->SetIsEnabled(false);

        Root->AddChild(std::move(Primary));
        Root->AddChild(std::move(Disabled));

        Penumbra::Platform::InputState Input;
        bool KeepRunning = true;
        while (KeepRunning) {
            KeepRunning = Window.PumpEventsAndBuildInput(Input);

            const SDL_FPoint Available = Window.GetLogicalWindowSize();
            const SDL_FPoint Desired = Root->Measure(Available);
            Root->Arrange({Theme.SpacingLarge, Theme.SpacingLarge, Desired.x, Desired.y});

            Root->UpdateInteractionState(Input);

            Renderer.BeginFrame(Theme.ColorBackgroundPrimary);
            Root->Draw(Renderer);
            Renderer.EndFrameAndPresent();
        }
    }

    Window.Shutdown();
    return 0;
}
