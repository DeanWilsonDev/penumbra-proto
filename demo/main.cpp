#include "DemoTheme.h"

#include "Penumbra/Platform/PlatformWindow.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Render/SdlTtfFontBackend.h"
#include "Penumbra/Widgets/Box.h"

#include <cstdio>
#include <memory>
#include <string>

namespace {

// Logical window size for the demo. A demo concern, never a Penumbra concern.
constexpr int WindowLogicalWidth  = 960;
constexpr int WindowLogicalHeight = 640;

constexpr const char* FontFileName = "JetBrainsMonoNerdFontMono-Regular.ttf";

using namespace Penumbra::Widgets;

EdgeInsets UniformInset(float Amount) { return {Amount, Amount, Amount, Amount}; }

// Builds a Box whose only visible size comes from its padding — a coloured slab
// used to make stacking, gaps, and margins visible at this milestone.
std::unique_ptr<Box> MakeSlab(const Demo::Theme& Theme, SDL_Color Fill, EdgeInsets Padding,
                              EdgeInsets Margin) {
    auto Slab = std::make_unique<Box>();
    Slab->Style.ColorBackground = Fill;
    Slab->Style.ColorBorder     = Theme.ColorBorderDefault;
    Slab->Style.BorderWidth     = Theme.BorderWidthDefault;
    Slab->Style.Padding         = Padding;
    Slab->Style.Margin          = Margin;
    return Slab;
}

} // namespace

int main() {
    Demo::Theme Theme;

    Penumbra::Platform::PlatformWindow Window;
    if (!Window.Initialise("Penumbra Demo", WindowLogicalWidth, WindowLogicalHeight)) {
        std::fprintf(stderr, "Failed to initialise platform window: %s\n", SDL_GetError());
        return 1;
    }

    std::printf("DPI scale factor: %.3f\n", Window.GetDpiScaleFactor());
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

        // A parent panel that vertically stacks two child slabs with a gap.
        // Different child sizes and margins prove margins are respected on both axes.
        auto Root = std::make_unique<Box>();
        Root->Style.ColorBackground = Theme.ColorSurfaceRaised;
        Root->Style.ColorBorder     = Theme.ColorBorderDefault;
        Root->Style.BorderWidth     = Theme.BorderWidthDefault;
        Root->Style.Padding         = UniformInset(Theme.SpacingLarge);
        Root->Layout                = LayoutMode::VerticalStack;
        Root->ChildGap              = Theme.SpacingMedium;

        Root->AddChild(MakeSlab(Theme, Theme.ColorAccent,
                                {64.0f, 32.0f, 64.0f, 32.0f},
                                {Theme.SpacingMedium, 0.0f, 0.0f, Theme.SpacingSmall}));
        Root->AddChild(MakeSlab(Theme, Theme.ColorTextPrimary,
                                {112.0f, 32.0f, 112.0f, 32.0f},
                                {Theme.SpacingLarge, Theme.SpacingSmall, 0.0f, 0.0f}));

        Penumbra::Platform::InputState Input;
        bool KeepRunning = true;
        while (KeepRunning) {
            KeepRunning = Window.PumpEventsAndBuildInput(Input);

            // Layout runs every frame: measure bottom-up, arrange top-down.
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
