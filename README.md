# Penumbra

A retained-mode C++20 UI framework on SDL3 (`SDL_Renderer` path), built to
validate the layered architecture described in
[`docs/penumbra_poc_spec.md`](docs/penumbra_poc_spec.md).

Contributor setup, architectural constraints, and validation guidance live in
[`DEVELOPING.md`](DEVELOPING.md).

## Layers

```
Demo App            stands in for Dawn; defines the Theme + resolvers
Penumbra::Widgets   Box, Button, Label, … (styled by values passed in)
Penumbra::Render    wraps SDL_Renderer; owns the IFontBackend (SDL_ttf)
Penumbra::Platform  the only code that calls SDL directly
```

The hard rule: no `SDL_Color` literal and no pixel literal anywhere under
`include/Penumbra/` or `src/Penumbra/`. All values live in the demo's `Theme`.

## Build & run

Requires CMake ≥ 3.24, a C++20 compiler, and SDL3, SDL3_ttf, and SDL3_image
installed with their CMake config packages.

```bash
# macOS (Homebrew)
brew install sdl3 sdl3_ttf sdl3_image

# Debian/Ubuntu — needs SDL3 packages (for example libsdl3-dev,
# libsdl3-ttf-dev, and libsdl3-image-dev),
# available on recent releases or from a backport/source build.
sudo apt install libsdl3-dev libsdl3-ttf-dev libsdl3-image-dev
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/demo/penumbra_demo
```

The demo loads its font from `demo/assets/` (vendored JetBrains Mono Nerd Font;
see `demo/assets/FONT-README.md` for its origin and license).
