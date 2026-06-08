#include "DemoResolvers.h"
#include "DemoTheme.h"

#include "Penumbra/Platform/PlatformWindow.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Render/SdlTtfFontBackend.h"
#include "Penumbra/Widgets/Box.h"
#include "Penumbra/Widgets/Button.h"
#include "Penumbra/Widgets/ScrollablePanel.h"

#include <cstdio>
#include <memory>
#include <string>

namespace {

// Logical window size for the demo. A demo concern, never a Penumbra concern.
constexpr int WindowLogicalWidth  = 960;
constexpr int WindowLogicalHeight = 640;

constexpr const char* FontFileName = "JetBrainsMonoNerdFontMono-Regular.ttf";

constexpr int RowCount = 20; // taller than the viewport, so the panel must scroll

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
    std::printf("Scroll the wheel over the panel; resize the window to re-run layout.\n");
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

        // A scrollable panel holding a tall column of full-width buttons. Alternating
        // backgrounds make the scroll motion and the clipped edges obvious.
        auto Root = std::make_unique<ScrollablePanel>();
        Root->Style           = Demo::ResolvePanelStyle(Theme);
        Root->ChildGap        = Theme.SpacingMedium;
        Root->CrossAlignment  = CrossAlign::Stretch;
        Root->WheelStepLogical = Theme.SpacingLarge * 3.0f;

        for (int Index = 0; Index < RowCount; ++Index) {
            auto RowStyle = Demo::ResolvePrimaryButtonStyle(Theme);
            if (Index % 2 == 1) {
                RowStyle.ColorBackground = Theme.ColorSurfaceRaised; // stripe
            }

            auto Row = std::make_unique<Button>();
            Row->ApplyStyle(RowStyle);
            Row->OnClicked = [Index]() {
                std::printf("Row %d clicked\n", Index);
                std::fflush(stdout);
            };
            Root->AddChild(std::move(Row));
        }

        Penumbra::Platform::InputState Input;
        bool KeepRunning = true;
        while (KeepRunning) {
            KeepRunning = Window.PumpEventsAndBuildInput(Input);

            // The viewport is recomputed from the live window size every frame, so a
            // window resize naturally re-runs measure + arrange.
            const SDL_FPoint Window2 = Window.GetLogicalWindowSize();
            const float Margin = Theme.SpacingLarge;
            const SDL_FRect Viewport{Margin, Margin, Window2.x - 2.0f * Margin,
                                     Window2.y - 2.0f * Margin};

            Root->Measure({Viewport.w, Viewport.h});
            Root->Arrange(Viewport);
            Root->UpdateInteractionState(Input);

            Renderer.BeginFrame(Theme.ColorBackgroundPrimary);
            Root->Draw(Renderer);
            Renderer.EndFrameAndPresent();
        }
    }

    Window.Shutdown();
    return 0;
}
