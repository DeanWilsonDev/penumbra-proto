#include "Penumbra/EntryPoint.h"
#include "Penumbra/Nyx/ApplicationBridge.h"
#include "Penumbra/Render/Color.h"
#include "Penumbra/Widgets/Box.h"

#include <cstdio>
#include <memory>
#include <string>

namespace {

// Auto-quits the demo after a bounded number of frames rather than requiring
// the user to close the window by hand -- also what makes this demo runnable
// unattended (e.g. under SDL_VIDEODRIVER=dummy for automated verification).
// Keeps unattended validation bounded; DemoApp.nyx now calls RequestQuit
// directly when Tick reports that this count has been reached.
constexpr int kAutoQuitAfterFrames = 300;

} // namespace

Penumbra::Application* CreateApplication() {
    ::nyx::host::NyxRuntime& Runtime = Penumbra::Nyx::GetRuntime();
    auto WidgetDescriptor = std::make_shared<const ::nyx::runtime::TypeDescriptor*>(nullptr);

    Runtime.RegisterFunction(
        "Log", [](std::vector<::nyx::runtime::Value> Args) -> ::nyx::runtime::Value {
            std::printf("[DemoApp.nyx] %s\n", ::nyx::host::FromValue<std::string>(Args[0]).c_str());
            return ::nyx::runtime::Value();
        });

    Runtime.RegisterFunction(
        "Tick", [](std::vector<::nyx::runtime::Value> Args) -> ::nyx::runtime::Value {
            static int FrameCount = 0;
            const float DeltaSeconds = ::nyx::host::FromValue<float>(Args[0]);
            ++FrameCount;

            // Once every ~2 seconds at 60fps -- proves DemoApp.nyx's own
            // OnUpdate is really being called every frame, not just once.
            if (FrameCount % 120 == 0) {
                std::printf("[DemoApp.nyx] OnUpdate has fired %d times so far (dt=%.4f)\n", FrameCount,
                            DeltaSeconds);
            }

            if (FrameCount == kAutoQuitAfterFrames) {
                std::printf("[demo_nyx] %d frames driven entirely from DemoApp.nyx -- auto-quitting\n",
                            FrameCount);
            }
            return ::nyx::host::ToValue(FrameCount >= kAutoQuitAfterFrames);
        });

    Runtime.RegisterFunction("CreateDemoRoot", [WidgetDescriptor](std::vector<::nyx::runtime::Value>) {
        auto Root = std::make_unique<Penumbra::Widgets::Box>();
        Root->Style.ColorBackground = Penumbra::Render::Color{40, 120, 200, 255};
        Root->OnPressed = []() {
            std::printf("[demo_nyx] script-mounted root received a click\n");
        };
        return ::nyx::host::ToValue(Root.release(), *WidgetDescriptor);
    });

    Runtime.RegisterFunction("InspectApplicationHandles", [](std::vector<::nyx::runtime::Value> Args) {
        const float Width = ::nyx::host::FromValue<float>(Args[0]);
        const float Height = ::nyx::host::FromValue<float>(Args[1]);
        const float DpiScale = ::nyx::host::FromValue<float>(Args[2]);
        (void)::nyx::host::FromValue<Penumbra::Render::IFontBackend*>(Args[3]);
        (void)::nyx::host::FromValue<Penumbra::LifecycleRegistry*>(Args[4]);
        (void)::nyx::host::FromValue<Penumbra::Widgets::WidgetBase*>(Args[5]);
        std::printf("[demo_nyx] Application methods exposed to Nyx: window %.0fx%.0f, DPI %.2f, "
                    "font/lifecycle/root handles valid\n",
                    Width, Height, DpiScale);
        return ::nyx::runtime::Value();
    });

    const std::string ScriptPath = std::string(DEMO_NYX_ASSET_DIR) + "/DemoApp.nyx";
    Penumbra::Application* Application =
        Penumbra::Nyx::LoadApplicationFromFile(ScriptPath, "DemoApplication");
    if (Application) {
        *WidgetDescriptor =
            std::get<std::shared_ptr<::nyx::runtime::HostObject>>(
                Runtime.Globals().at("PenumbraWidget").data)
                ->descriptor;
    }
    return Application;
}
