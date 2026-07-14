# Penumbra — `Image` Widget and `IImageBackend` (required for Iris `<Image>` primitive)

> **Scope:** Minimal image loading and rendering support. Mirrors the shape of the
> existing `IFontBackend`/`SdlTtfFontBackend` abstraction. No asset management, no
> caching, no async loading — just enough to render an image from a file path without
> crashing.
>
> **Why now:** `<Image>` is an Iris Core primitive. Every backend is required to implement
> it. Stage 2 cannot claim to cover Core primitives without it.
>
> **Status:** Implemented as `ImageWidget` (not `Image` — the earlier `Box`-derived
> `Image`/`Image::Builder` from the first pass has been removed and replaced). Two
> deliberate adaptations from the pseudocode below, both because it was written without
> Penumbra's exact API in hand: `ImageWidget::Measure`/`Arrange`/`Draw`/
> `UpdateInteractionState` use Penumbra's real `Point`/`Rect`/`Render::Renderer`/
> `Platform::InputState` signatures, not the `AvailableSize`/raw-`SDL_Renderer*` shapes
> shown here; and `ImageWidget` still implements `UpdateInteractionState` (pure virtual on
> the real `WidgetBase`) by dispatching the inherited `OnPressed`/etc. callbacks the same
> way `Box` does, since those already exist on every widget project-wide.

---

## 1. `IImageBackend` interface

Mirrors `IFontBackend` in shape and purpose — an abstraction over the concrete image
loading library so the backend can be swapped without touching widget code.

### `include/Penumbra/Backends/IImageBackend.h`

```cpp
class IImageBackend {
public:
    virtual SDL_Texture* LoadTexture(SDL_Renderer* Renderer,
                                     std::string_view FilePath) = 0;
    virtual void ReleaseTexture(SDL_Texture* Texture) = 0;
    virtual ~IImageBackend() = default;
};
```

---

## 2. `SdlImageBackend` — concrete implementation

Loads PNG and JPG from disk via `SDL_image`. Returns an `SDL_Texture*` owned by the
caller. `ReleaseTexture` calls `SDL_DestroyTexture`.

### `include/Penumbra/Backends/SdlImageBackend.h`
### `src/Penumbra/Backends/SdlImageBackend.cpp`

```cpp
class SdlImageBackend : public IImageBackend {
public:
    SDL_Texture* LoadTexture(SDL_Renderer* Renderer,
                              std::string_view FilePath) override;
    void ReleaseTexture(SDL_Texture* Texture) override;
};
```

**Implementation note:** Use `IMG_Load` from `SDL_image` followed by
`SDL_CreateTextureFromSurface`. Free the intermediate surface immediately after texture
creation.

---

## 3. `ImageWidget` widget

A `WidgetBase` subclass that holds an `SDL_Texture*` and renders it via
`SDL_RenderTexture` during `Draw`.

### `include/Penumbra/Widgets/ImageWidget.h`
### `src/Penumbra/Widgets/ImageWidget.cpp`

```cpp
class ImageWidget : public WidgetBase {
public:
    std::string FilePath;
    SDL_Texture* Texture = nullptr;

    void LoadFrom(IImageBackend& Backend, SDL_Renderer* Renderer);
    void Measure(AvailableSize Available) override;
    void Arrange(Rect Bounds) override;
    void Draw(SDL_Renderer* Renderer) override;

    // Builder
    struct Builder {
        Builder& src(std::string_view FilePath);
        Builder& className(std::string_view ClassName);
        std::unique_ptr<ImageWidget> build();
    private:
        std::string FilePath;
        std::string ClassName;
    };
};
```

**Measure behaviour:** If the texture is loaded, report the texture's natural pixel
dimensions. If not yet loaded, report zero size.

**Draw behaviour:** `SDL_RenderTexture` into the widget's arranged bounds. Stretches to
fill — aspect-ratio preservation is a Lustre concern deferred to Stage 4.

---

## 4. Builder method — prop name mapping

| Builder method | Maps to Iris prop |
|---|---|
| `.src(std::string_view)` | `src` |
| `.className(std::string_view)` | `class` |

---

## 5. Done when

- `IImageBackend` interface exists at `include/Penumbra/Backends/IImageBackend.h`
- `SdlImageBackend` loads PNG/JPG and returns a valid `SDL_Texture*`
- `ImageWidget` renders a loaded texture into its arranged bounds
- `ImageWidget::Builder` follows the same fluent pattern as `Box::Builder`
- `GetChildCount` returns 0, `GetChildAt` returns nullptr (image is a leaf widget)
- Existing consumers (Dawn, Pharos) are unaffected — purely additive
