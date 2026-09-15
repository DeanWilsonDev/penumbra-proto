#include "Penumbra/Nyx/ApplicationBridge.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>
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
    Penumbra::Point* GetWindowLogicalSizeForNyx() {
        WindowLogicalSize = Penumbra::Application::GetWindowLogicalSize();
        return &WindowLogicalSize;
    }

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

private:
    Penumbra::Point WindowLogicalSize;
};

} // namespace nyx::host

namespace Penumbra::Nyx {

namespace {

float PointX(const Point& Value) { return Value.X; }
float PointY(const Point& Value) { return Value.Y; }

Point* GetWindowLogicalSizeForNyx(Application& Self) {
    auto& Bridge = static_cast<::nyx::host::NyxBridge<Application>&>(Self);
    return Bridge.GetWindowLogicalSizeForNyx();
}

Render::IFontBackend* GetFontBackendForNyx(Application& Self) { return &Self.GetFontBackend(); }

LifecycleRegistry* GetLifecycleRegistryForNyx(Application& Self) { return &Self.GetLifecycleRegistry(); }

void SetRootWidgetFromNyx(Application& Self, Widgets::WidgetBase* Root) {
    Self.SetRootWidget(std::unique_ptr<Widgets::WidgetBase>(Root));
}

template <typename T>
const ::nyx::runtime::TypeDescriptor* RegisterOpaqueType(::nyx::host::NyxRuntime& Runtime,
                                                          const std::string& Name) {
    Runtime.RegisterType<T>(Name);
    return std::get<std::shared_ptr<::nyx::runtime::HostObject>>(Runtime.Globals().at(Name).data)->descriptor;
}

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

struct ExternalHostState {
    bool TypeRegistered = false;
    std::vector<std::shared_ptr<::nyx::interpreter::Interpreter>> Interpreters;
};

ExternalHostState& GetExternalHostState(::nyx::host::NyxRuntime& Runtime) {
    static std::unordered_map<::nyx::host::NyxRuntime*, ExternalHostState> Hosts;
    return Hosts[&Runtime];
}

Application* MountApplication(
    ::nyx::host::NyxRuntime& Runtime, bool& TypeRegistered,
    std::vector<std::shared_ptr<::nyx::interpreter::Interpreter>>& Interpreters,
    const std::string& Source, const std::string& Filename, const std::string& ApplicationClassName) {
    if (!TypeRegistered) {
        Runtime.RegisterType<Point>("PenumbraPoint").Method("X", &PointX).Method("Y", &PointY);
        const auto* PointDescriptor =
            std::get<std::shared_ptr<::nyx::runtime::HostObject>>(Runtime.Globals().at("PenumbraPoint").data)
                ->descriptor;
        const auto* FontBackendDescriptor =
            RegisterOpaqueType<Render::IFontBackend>(Runtime, "PenumbraFontBackend");
        const auto* LifecycleRegistryDescriptor =
            RegisterOpaqueType<LifecycleRegistry>(Runtime, "PenumbraLifecycleRegistry");
        const auto* WidgetDescriptor = RegisterOpaqueType<Widgets::WidgetBase>(Runtime, "PenumbraWidget");

        Runtime.RegisterInheritableType<Application>("Application")
            .Method("RequestQuit", &Application::RequestQuit)
            .PointerMethod("GetWindowLogicalSize", &GetWindowLogicalSizeForNyx, PointDescriptor)
            .Method("GetDpiScaleFactor", &Application::GetDpiScaleFactor)
            .PointerMethod("GetFontBackend", &GetFontBackendForNyx, FontBackendDescriptor)
            .Method("SetTextInputActive", &Application::SetTextInputActive)
            .Method("SetRootWidget", &SetRootWidgetFromNyx)
            .PointerMethod("GetRootWidget", &Application::GetRootWidget, WidgetDescriptor)
            .Method("GetRootWidgetConsumedInputThisFrame",
                    &Application::GetRootWidgetConsumedInputThisFrame)
            .PointerMethod("GetLifecycleRegistry", &GetLifecycleRegistryForNyx,
                           LifecycleRegistryDescriptor)
            .Override("OnStart", +[](Application& Self) -> bool { return Self.Application::OnStart(); })
            .Override("OnUpdate",
                      +[](Application& Self, float DeltaSeconds) { Self.Application::OnUpdate(DeltaSeconds); })
            .Override("OnShutdown", +[](Application& Self) { Self.Application::OnShutdown(); })
            .Override("OnDpiScaleChanged", +[](Application& Self, float NewDpiScaleFactor) {
                Self.Application::OnDpiScaleChanged(NewDpiScaleFactor);
            });
        TypeRegistered = true;
    }

    try {
        auto Scope = Runtime.MountBridged<Application>(Source, Filename, ApplicationClassName);
        Interpreters.push_back(Scope.interpreter);
        return &Scope.Get();
    } catch (const std::exception& Error) {
        std::fprintf(stderr, "Penumbra::Nyx::LoadApplication: %s: %s\n", Filename.c_str(), Error.what());
        return nullptr;
    }
}

} // namespace

Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName) {
    BridgeHost& Host = GetBridgeHost();
    return MountApplication(Host.Runtime, Host.TypeRegistered, Host.Interpreters, Source, Filename,
                            ApplicationClassName);
}

Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName,
                             ::nyx::host::NyxRuntime& ExternalRuntime) {
    ExternalHostState& Host = GetExternalHostState(ExternalRuntime);
    return MountApplication(ExternalRuntime, Host.TypeRegistered, Host.Interpreters, Source, Filename,
                            ApplicationClassName);
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

Application* LoadApplicationFromFile(const std::filesystem::path& Path,
                                     const std::string& ApplicationClassName,
                                     ::nyx::host::NyxRuntime& ExternalRuntime) {
    std::ifstream File(Path);
    if (!File) {
        std::fprintf(stderr, "Penumbra::Nyx::LoadApplicationFromFile: cannot open '%s'\n", Path.string().c_str());
        return nullptr;
    }
    std::ostringstream Contents;
    Contents << File.rdbuf();
    return LoadApplication(Contents.str(), Path.filename().string(), ApplicationClassName, ExternalRuntime);
}

::nyx::host::NyxRuntime& GetRuntime() {
    return GetBridgeHost().Runtime;
}

} // namespace Penumbra::Nyx
