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
    # external/firefly) and Amanuensis (Firefly's own private JSON
    # dependency, vendored two levels deep at
    # external/firefly/external/amanuensis) are compiled directly from their
    # own source lists below instead of add_subdirectory()ing their vendored
    # trees -- NOT running their CMakeLists.txt at all, on purpose.
    #
    # add_subdirectory(external/firefly) previously ran Firefly's own
    # CMakeLists.txt, which unconditionally add_subdirectory()s its own
    # external/amanuensis with no `if(TARGET amanuensis)` guard at whatever
    # commit nyx-proto's own submodule pin happened to point to. A consumer
    # that already has its own `amanuensis` target declared before pulling
    # in Penumbra (pharos-proto, via iris-proto's vendored copy) hit a hard
    # `add_library` name collision or silently got the wrong copy's headers
    # depending on fetch order -- see docs/next_steps.md's "PENUMBRA_WITH_NYX's
    # own build wiring collides with a consumer's amanuensis target" entry
    # for the full incident. iris-proto's own CMakeLists.txt already
    # documents and avoids this identical trap for its own nyx-proto
    # dependency by never add_subdirectory()ing the vendored tree either --
    # this mirrors that approach one dependency further down.
    #
    # Each target below is guarded with `if(NOT TARGET ...)` so a consumer
    # that's already declared amanuensis/firefly/nyx-core itself (from its
    # own, possibly differently-vendored copy) wins and gets reused here
    # instead of a second, colliding declaration -- whichever copy is
    # declared first in the whole build is the one everyone links against.
    if(NOT TARGET amanuensis)
        add_library(amanuensis STATIC
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/json.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/io/reader.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/io/writer.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/io/writer-context.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/io/parser.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/serialization/serialization.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/converter.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/ordered-map.cpp
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/src/value-traits.cpp)
        target_include_directories(amanuensis PUBLIC
            ${nyx_SOURCE_DIR}/external/firefly/external/amanuensis/include)
        target_compile_features(amanuensis PUBLIC cxx_std_20)
    endif()

    if(NOT TARGET firefly)
        add_library(firefly STATIC
            ${nyx_SOURCE_DIR}/external/firefly/src/logger.cpp
            ${nyx_SOURCE_DIR}/external/firefly/src/log-registry.cpp
            ${nyx_SOURCE_DIR}/external/firefly/src/sinks/plain-text-sink.cpp
            ${nyx_SOURCE_DIR}/external/firefly/src/sinks/json-sink.cpp
            ${nyx_SOURCE_DIR}/external/firefly/src/sinks/ndjson-sink.cpp
            ${nyx_SOURCE_DIR}/external/firefly/src/sinks/csv-sink.cpp)
        target_include_directories(firefly PUBLIC ${nyx_SOURCE_DIR}/external/firefly/include)
        target_compile_features(firefly PUBLIC cxx_std_20)
        target_precompile_headers(firefly PRIVATE ${nyx_SOURCE_DIR}/external/firefly/include/pch.hpp)
        target_link_libraries(firefly PRIVATE amanuensis)
    endif()

    # nyx-core itself -- same source list as nyx-proto/CMakeLists.txt's own
    # `nyx-core` target. Built at C++26 (nyx-proto's own required standard),
    # independent of Penumbra's own CMAKE_CXX_STANDARD 20; consuming TUs
    # (src/Penumbra/Nyx/ApplicationBridge.cpp) include nyx-core's public
    # headers at Penumbra's own standard without issue, same as
    # pharos-proto's own C++20 code consuming the same headers. Guarded like
    # amanuensis/firefly above, in case a consumer already declared its own
    # nyx-core target (e.g. by FetchContent-ing nyx-proto itself directly,
    # the way pharos-proto's own cmake/Dependencies.cmake does).
    #
    # Source list bumped to add interpreter/resolver.cpp alongside nyx-proto's e28c957
    # (decision-log.md §8.7/§8.8's field-slot-caching fix): Interpreter's ctor now
    # unconditionally calls the new ResolveFieldNames, so omitting this file was a link
    # error for penumbra_demo_nyx (and any other consumer, since this target usually wins
    # the `if(NOT TARGET nyx-core)` race over a consumer's own copy -- see pharos-proto's
    # cmake/Dependencies.cmake, which hit the identical gap and applied the identical fix).
    if(NOT TARGET nyx-core)
        add_library(nyx-core STATIC
            ${nyx_SOURCE_DIR}/src/lexer/token.cpp
            ${nyx_SOURCE_DIR}/src/lexer/lexer.cpp
            ${nyx_SOURCE_DIR}/src/parser/parser.cpp
            ${nyx_SOURCE_DIR}/src/runtime/value.cpp
            ${nyx_SOURCE_DIR}/src/runtime/environment.cpp
            ${nyx_SOURCE_DIR}/src/interpreter/decl-registry.cpp
            ${nyx_SOURCE_DIR}/src/interpreter/resolver.cpp
            ${nyx_SOURCE_DIR}/src/interpreter/interpreter.cpp
            ${nyx_SOURCE_DIR}/src/host/logging.cpp
            ${nyx_SOURCE_DIR}/src/host/stub-types.cpp)
        target_include_directories(nyx-core PUBLIC ${nyx_SOURCE_DIR}/src)
        target_link_libraries(nyx-core PRIVATE firefly)
        set_target_properties(nyx-core PROPERTIES
            CXX_STANDARD 26
            CXX_STANDARD_REQUIRED ON)
    endif()

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
