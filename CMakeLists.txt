cmake_minimum_required(VERSION 3.24)

project(Penumbra
    VERSION 0.0.1
    DESCRIPTION "Penumbra — retained-mode C++20 UI framework on SDL3"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# The expected path: SDL3 / SDL3_ttf / SDL3_image installed with CMake config packages.
find_package(SDL3       CONFIG REQUIRED)
find_package(SDL3_ttf   CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Penumbra — static library. Nothing in here references demo/.
# ---------------------------------------------------------------------------
add_library(penumbra STATIC
    src/Penumbra/Application.cpp
    src/Penumbra/Platform/PlatformWindow.cpp
    src/Penumbra/Render/Renderer.cpp
    src/Penumbra/Render/SdlTtfFontBackend.cpp
    src/Penumbra/Backends/SdlImageBackend.cpp
    src/Penumbra/Widgets/Box.cpp
    src/Penumbra/Widgets/InlineContainer.cpp
    src/Penumbra/Widgets/ImageWidget.cpp
    src/Penumbra/Widgets/IconWidget.cpp
    src/Penumbra/Widgets/Button.cpp
    src/Penumbra/Widgets/ScrollablePanel.cpp
    src/Penumbra/Widgets/Label.cpp
    src/Penumbra/Widgets/Checkbox.cpp
    src/Penumbra/Widgets/NumericDrag.cpp
    src/Penumbra/Widgets/TextInput.cpp
    src/Penumbra/Widgets/ViewportWidget.cpp
    src/Penumbra/Widgets/SplitPanel.cpp
    src/Penumbra/Widgets/OverlayHost.cpp)

target_include_directories(penumbra PUBLIC include)

# SDL3 is linked PUBLIC because a couple of internals (ViewportWidget's scene
# texture, PlatformWindow's SDL_Renderer*) are legitimate exceptions to
# Penumbra's own "no SDL above Platform" rule; the demo does not need an SDL
# include for ordinary widget/color/geometry code (Color/Point/Rect/Key cover it).
# SDL3_ttf is linked now though only used from Milestone 1 onward.
target_link_libraries(penumbra PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)

# SDL3_image is linked PRIVATE: unlike SDL3/SDL3_ttf, no public Penumbra header
# names an SDL3_image type (IImageBackend/SdlImageBackend/ImageWidget only use
# SDL_Texture/SDL_Renderer from SDL3 core) -- it's an implementation detail
# entirely contained in SdlImageBackend.cpp, so consumers don't need it on their
# own include/link path.
target_link_libraries(penumbra PRIVATE SDL3_image::SDL3_image)

# ---------------------------------------------------------------------------
# Optional Nyx bridge -- makes Penumbra::Application inheritable from a .nyx
# script (nyx-proto's RegisterInheritableType/NyxBridge mechanism,
# include/Penumbra/Nyx/ApplicationBridge.h, docs/next_steps.md's "Application
# base class" entry). Off by default: nothing else in Penumbra (including the
# demo) depends on nyx-proto, and enabling this pulls in a whole second
# interpreter plus its own C++26-only build -- a consumer opts in explicitly
# with -DPENUMBRA_WITH_NYX=ON.
# ---------------------------------------------------------------------------
option(PENUMBRA_WITH_NYX "Build Penumbra::Nyx::ApplicationBridge (requires nyx-proto)" OFF)

if(PENUMBRA_WITH_NYX)
    include(FetchContent)

    # Tracks `main`, not a pinned commit -- same trade-off pharos-proto's own
    # cmake/Dependencies.cmake documents for its own nyx-proto dependency:
    # nyx-proto is under active, alongside co-development, no released tag yet.
    #
    # Populate-source-only, not FetchContent_MakeAvailable/add_subdirectory of
    # nyx-proto's own top-level CMakeLists.txt -- that file unconditionally
    # add_subdirectory()s external/cimmerian and tests/, neither of which
    # Penumbra needs just to consume nyx-core as a library. nyx-core and its
    # own Firefly logging dependency are declared directly below instead,
    # mirroring nyx-proto/CMakeLists.txt's own `nyx-core` target.
    FetchContent_Declare(
        nyx
        GIT_REPOSITORY https://github.com/DeanWilsonDev/nyx-proto.git
        GIT_TAG        main
    )
    FetchContent_GetProperties(nyx)
    if(NOT nyx_POPULATED)
        FetchContent_Populate(nyx)
    endif()

    # Firefly (nyx-core's own logging dependency, vendored at
    # external/firefly) guards its own test/cimmerian subdirectories behind
    # an IS_ROOT_PROJECT check (external/firefly/CMakeLists.txt), so
    # add_subdirectory()ing it here -- not as the CMake root -- pulls in only
    # the `firefly` library target itself. AMANUENSIS_BUILD_TESTS OFF for the
    # same reason nyx-proto's own top-level CMakeLists.txt forces it off
    # before this same add_subdirectory call: Amanuensis's own test suite
    # (which vendors its own copy of Cimmerian) is unwanted here, and nothing
    # else in this build declares an `amanuensis` target first (unlike
    # pharos-proto, which already gets one from `iris` and has to work around
    # the collision -- not a concern here).
    set(AMANUENSIS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${nyx_SOURCE_DIR}/external/firefly ${CMAKE_CURRENT_BINARY_DIR}/_deps/firefly-build)

    # nyx-core itself -- same source list as nyx-proto/CMakeLists.txt's own
    # `nyx-core` target. Built at C++26 (nyx-proto's own required standard),
    # independent of Penumbra's own CMAKE_CXX_STANDARD 20; consuming TUs
    # (src/Penumbra/Nyx/ApplicationBridge.cpp) include nyx-core's public
    # headers at Penumbra's own standard without issue, same as
    # pharos-proto's own C++20 code consuming the same headers.
    add_library(nyx-core STATIC
        ${nyx_SOURCE_DIR}/src/lexer/token.cpp
        ${nyx_SOURCE_DIR}/src/lexer/lexer.cpp
        ${nyx_SOURCE_DIR}/src/parser/parser.cpp
        ${nyx_SOURCE_DIR}/src/runtime/value.cpp
        ${nyx_SOURCE_DIR}/src/runtime/environment.cpp
        ${nyx_SOURCE_DIR}/src/interpreter/decl-registry.cpp
        ${nyx_SOURCE_DIR}/src/interpreter/interpreter.cpp
        ${nyx_SOURCE_DIR}/src/host/logging.cpp
        ${nyx_SOURCE_DIR}/src/host/stub-types.cpp)
    target_include_directories(nyx-core PUBLIC ${nyx_SOURCE_DIR}/src)
    target_link_libraries(nyx-core PRIVATE firefly)
    set_target_properties(nyx-core PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED ON)

    add_library(penumbra_nyx_bridge STATIC
        src/Penumbra/Nyx/ApplicationBridge.cpp)
    target_link_libraries(penumbra_nyx_bridge PUBLIC penumbra nyx-core)

    # demo_nyx — mini demo proving the bridge end to end: DemoApplication
    # (demo_nyx/assets/DemoApp.nyx) is a real .nyx class extending Application,
    # not a C++ subclass -- window/renderer construction and the frame loop are
    # entirely owned by Penumbra::Application::Run(), reached through
    # Penumbra/EntryPoint.h exactly like any plain C++ app.
    add_executable(penumbra_demo_nyx
        demo_nyx/main.cpp)
    target_link_libraries(penumbra_demo_nyx PRIVATE penumbra_nyx_bridge)
    set_target_properties(penumbra_demo_nyx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo_nyx")
    target_compile_definitions(penumbra_demo_nyx PRIVATE
        DEMO_NYX_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/demo_nyx/assets")
endif()

# ---------------------------------------------------------------------------
# Demo — executable. The ONLY target that defines values. Links penumbra.
# ---------------------------------------------------------------------------
add_executable(penumbra_demo
    demo/main.cpp
    demo/DemoTheme.cpp
    demo/DemoResolvers.cpp)

target_link_libraries(penumbra_demo PRIVATE penumbra)
set_target_properties(penumbra_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo")

# Absolute path to the demo's vendored assets, so the binary finds its font
# regardless of the working directory it is launched from. A demo concern only.
target_compile_definitions(penumbra_demo PRIVATE
    DEMO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/demo/assets")
