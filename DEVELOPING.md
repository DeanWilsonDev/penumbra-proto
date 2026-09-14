# Developing Penumbra

This is the shared development guide for people and coding agents. Keep durable
project knowledge here rather than in instructions for a specific tool.

## Project shape

Penumbra is a retained-mode C++20 UI framework built on SDL3's `SDL_Renderer`
path. The repository contains:

- `include/Penumbra/`: public headers, grouped by platform, rendering,
  backends, widgets, animation, and the optional Nyx bridge.
- `src/Penumbra/`: implementations. Keep paths paired with their public
  headers where applicable.
- `demo/`: the normal C++ demo and the theme/style values used to exercise the
  library.
- `demo_nyx/`: the optional Nyx-backed demo.
- `docs`: a tracked symlink to the separate `penumbra-ui-library`
  documentation repository. It may be absent or broken in a standalone clone.
  Treat its contents as design context, not as files owned by this repository.

`penumbra` is the core static library. `penumbra_demo` is the default runnable
target. With `PENUMBRA_WITH_NYX=ON`, CMake also builds
`penumbra_nyx_bridge` and `penumbra_demo_nyx` and fetches `nyx-proto`.

## Architectural constraints

- Dependencies flow from app/demo to widgets to rendering to platform.
- SDL window and event-loop calls belong under `Penumbra::Platform`.
  Rendering and backend code may use SDL types where their interfaces require
  them; do not spread SDL concerns into ordinary widget or demo composition
  code.
- The framework supplies behavior and style slots, not an aesthetic. Do not add
  hard-coded colors, spacing, sizes, or other theme values under
  `include/Penumbra/` or `src/Penumbra/`. Put concrete visual values in the
  consuming app; this repository's example is `demo/DemoTheme.*`.
- Preserve the retained-mode frame flow: measure, arrange, update interaction
  state, then draw.
- Keep the core library independent of `demo/` and Nyx. Nyx integration must
  remain behind `PENUMBRA_WITH_NYX`.
- Public names use `PascalCase`, including methods and local variables. The
  namespace is `Penumbra`; do not introduce Unreal-style type prefixes.

The proof-of-concept specification in `docs/penumbra_poc_spec.md` gives deeper
design context when the documentation checkout is available. Prefer the
current code and CMake configuration where an older design document disagrees.

## Build and validation

Prerequisites are CMake 3.24 or newer, a C++20 compiler, and SDL3, SDL3_ttf,
and SDL3_image with CMake package configurations.

Use an out-of-tree build directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/demo/penumbra_demo
```

There is currently no automated test target. For normal code changes, a clean
configure and build is the minimum validation. Run the demo when behavior or
rendering changes and report anything that could not be exercised in the
current environment.

Validate the optional integration separately when changing `include/Penumbra/Nyx/`,
`src/Penumbra/Nyx/`, its CMake wiring, or shared APIs used by that bridge:

```sh
cmake -S . -B build-nyx -DCMAKE_BUILD_TYPE=Debug -DPENUMBRA_WITH_NYX=ON
cmake --build build-nyx
./build-nyx/demo_nyx/penumbra_demo_nyx
```

The Nyx configure step needs network access unless CMake already has the
dependency available.

## Change hygiene

- Add a new public `.h` and implementation `.cpp` to the matching directories,
  and list new compiled sources explicitly in `CMakeLists.txt`.
- Do not commit `build/` or `build-*` output.
- Do not edit through the `docs` symlink as part of a Penumbra code change;
  documentation changes belong to its target repository and should be called
  out as a separate cross-repository task.
- Keep patches focused and preserve unrelated local changes.
- Update this guide when build commands, boundaries, or repository structure
  change. Put tool-only permissions and UI preferences in tool-specific local
  settings, not here.
