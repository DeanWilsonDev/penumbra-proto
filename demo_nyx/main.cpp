#include "Penumbra/EntryPoint.h"
#include "Penumbra/Nyx/ApplicationBridge.h"

#include <cstdio>
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

            if (FrameCount == kAutoQuitAfterFrames && GApplication) {
                std::printf("[demo_nyx] %d frames driven entirely from DemoApp.nyx -- auto-quitting\n",
                            FrameCount);
                GApplication->RequestQuit();
            }
            return ::nyx::runtime::Value();
        });

    const std::string ScriptPath = std::string(DEMO_NYX_ASSET_DIR) + "/DemoApp.nyx";
    GApplication = Penumbra::Nyx::LoadApplicationFromFile(ScriptPath, "DemoApplication");
    return GApplication;
}
