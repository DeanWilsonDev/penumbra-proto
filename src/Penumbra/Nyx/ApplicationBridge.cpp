#include "Penumbra/Nyx/ApplicationBridge.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

// The NyxBridge<Penumbra::Application> specialization lives here, not in the
// public header: nothing outside this file needs to name it directly today
// (LoadApplication/LoadApplicationFromFile below are the only entry points),
// and nyx-proto's own convention (nyx-proto/tests/host_test.cpp) is to define
// each type's specialization right next to wherever it gets registered.
namespace nyx::host {

template <>
class NyxBridge<Penumbra::Application> : public Penumbra::Application, public NyxBridgeBase {
public:
    bool OnStart() override {
        if (std::optional<runtime::Value> Result = Invoke("OnStart")) {
            return FromValue<bool>(*Result);
        }
        return Penumbra::Application::OnStart();
    }

    void OnUpdate(float DeltaSeconds) override {
        if (!Invoke("OnUpdate", DeltaSeconds)) {
            Penumbra::Application::OnUpdate(DeltaSeconds);
        }
    }

    void OnShutdown() override {
        if (!Invoke("OnShutdown")) {
            Penumbra::Application::OnShutdown();
        }
    }

    void OnDpiScaleChanged(float NewDpiScaleFactor) override {
        if (!Invoke("OnDpiScaleChanged", NewDpiScaleFactor)) {
            Penumbra::Application::OnDpiScaleChanged(NewDpiScaleFactor);
        }
    }
};

} // namespace nyx::host

namespace Penumbra::Nyx {

namespace {

// Keeps the NyxRuntime (owns the InheritableTypeDescriptor a bridged
// instance's Nyx super calls stay resolvable against) and every Interpreter
// MountBridged hands back (the bridge's own NyxBridgeBase::interp_ is a raw
// pointer into it) alive for the whole process -- deliberately never released
// before static destruction at exit, matching this factory's own contract
// (Penumbra/Nyx/ApplicationBridge.h: "suitable to return directly from
// CreateApplication()", i.e. called once, from main(), for the app's whole
// lifetime). A caller with a shorter-lived or repeated-mount use case should
// drive ::nyx::host::NyxRuntime/RegisterInheritableType/MountBridged directly
// instead of going through this convenience wrapper.
struct BridgeHost {
    ::nyx::host::NyxRuntime Runtime;
    bool TypeRegistered = false;
    std::vector<std::shared_ptr<::nyx::interpreter::Interpreter>> Interpreters;
};

BridgeHost& GetBridgeHost() {
    static BridgeHost Host;
    return Host;
}

} // namespace

Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName) {
    BridgeHost& Host = GetBridgeHost();

    if (!Host.TypeRegistered) {
        Host.Runtime.RegisterInheritableType<Application>("Application")
            .Override("OnStart", +[](Application& Self) -> bool { return Self.Application::OnStart(); })
            .Override("OnUpdate",
                     +[](Application& Self, float DeltaSeconds) { Self.Application::OnUpdate(DeltaSeconds); })
            .Override("OnShutdown", +[](Application& Self) { Self.Application::OnShutdown(); })
            .Override("OnDpiScaleChanged", +[](Application& Self, float NewDpiScaleFactor) {
                Self.Application::OnDpiScaleChanged(NewDpiScaleFactor);
            });
        Host.TypeRegistered = true;
    }

    try {
        auto Scope = Host.Runtime.MountBridged<Application>(Source, Filename, ApplicationClassName);
        Host.Interpreters.push_back(Scope.interpreter);
        return &Scope.Get();
    } catch (const std::exception& Error) {
        std::fprintf(stderr, "Penumbra::Nyx::LoadApplication: %s: %s\n", Filename.c_str(), Error.what());
        return nullptr;
    }
}

Application* LoadApplicationFromFile(const std::filesystem::path& Path,
                                     const std::string& ApplicationClassName) {
    std::ifstream File(Path);
    if (!File) {
        std::fprintf(stderr, "Penumbra::Nyx::LoadApplicationFromFile: cannot open '%s'\n", Path.string().c_str());
        return nullptr;
    }
    std::ostringstream Contents;
    Contents << File.rdbuf();
    return LoadApplication(Contents.str(), Path.filename().string(), ApplicationClassName);
}

::nyx::host::NyxRuntime& GetRuntime() {
    return GetBridgeHost().Runtime;
}

} // namespace Penumbra::Nyx
