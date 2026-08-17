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
// Illustrates the one thing a Nyx script *can't* do directly on this bridge
// (RequestQuit isn't a bridged hook, see docs/next_steps.md's Nyx bridge
// entry) but a host-registered callback the script calls into can, on the
// Application's behalf.
constexpr int kAutoQuitAfterFrames = 300;

// Set once CreateApplication() below has a real Application* to call
// RequestQuit() on -- the Tick callback is registered (and may start firing,
// once the frame loop starts) before that pointer exists, so this starts
// null and Tick checks it before touching it.
Penumbra::Application* GApplication = nullptr;

} // namespace

Penumbra::Application* CreateApplication() {
    Penumbra::Nyx::GetRuntime().RegisterFunction(
        "Log", [](std::vector<::nyx::runtime::Value> Args) -> ::nyx::runtime::Value {
            std::printf("[DemoApp.nyx] %s\n", ::nyx::host::FromValue<std::string>(Args[0]).c_str());
            return ::nyx::runtime::Value();
        });

    Penumbra::Nyx::GetRuntime().RegisterFunction(
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

            // Proves Application::SetRootWidget's automatic Measure/Arrange pass ran --
            // this Tick call itself fires from inside OnUpdate, i.e. *before* Run()'s
            // own automatic pass for this same frame, so frame 1's arrangement isn't
            // observable until frame 2's Tick call reads it back: by then the root Box
            // (added below, filling the window) already has a real arranged size,
            // without this file (or DemoApp.nyx) ever calling Measure/Arrange itself.
            if (FrameCount == 2 && GApplication && GApplication->GetRootWidget()) {
                const Penumbra::Rect Arranged = GApplication->GetRootWidget()->GetArrangedRect();
                std::printf("[demo_nyx] root widget auto-arranged to %.0fx%.0f (observed at frame 2, "
                            "set during frame 1) -- SetRootWidget's automatic "
                            "Measure/Arrange/Draw pass is live\n",
                            Arranged.W, Arranged.H);
            }

            if (FrameCount == kAutoQuitAfterFrames && GApplication) {
                std::printf("[demo_nyx] %d frames driven entirely from DemoApp.nyx -- auto-quitting\n",
                            FrameCount);
                GApplication->RequestQuit();
            }
            return ::nyx::runtime::Value();
        });

    const std::string ScriptPath = std::string(DEMO_NYX_ASSET_DIR) + "/DemoApp.nyx";
    GApplication = Penumbra::Nyx::LoadApplicationFromFile(ScriptPath, "DemoApplication");

    // Exercises Application::SetRootWidget end-to-end from exactly the caller shape
    // it was designed for: a host that only has an Application* obtained from
    // LoadApplicationFromFile (DemoApplication, above, has no C++ subclassing point
    // of its own -- it's authored entirely in DemoApp.nyx). No OnRender override or
    // SetOnRenderHook is registered anywhere in this file: this solid-colour Box is
    // the demo's *entire* visual, drawn purely by Run()'s own automatic
    // Measure/Arrange/UpdateInteractionState/Draw pass. Clicking it (OnPressed) also
    // proves the automatic UpdateInteractionState call really reaches widget
    // callbacks, not just Measure/Arrange.
    if (GApplication) {
        auto Root = std::make_unique<Penumbra::Widgets::Box>();
        Root->Style.ColorBackground = Penumbra::Render::Color{40, 120, 200, 255};
        Root->OnPressed             = []() {
            std::printf("[demo_nyx] root widget clicked -- SetRootWidget's automatic "
                        "UpdateInteractionState pass reached OnPressed\n");
        };
        GApplication->SetRootWidget(std::move(Root));
    }

    return GApplication;
}
