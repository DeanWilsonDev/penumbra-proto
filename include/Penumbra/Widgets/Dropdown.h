#pragma once

#include "Penumbra/Render/IFontBackend.h"
#include "Penumbra/Widgets/Box.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Penumbra::Widgets {

class OverlayHost;
struct DropdownState;

struct DropdownItem {
    std::string Label;
    std::function<void(Render::Renderer&, Rect, Render::Color)> DrawLeadingVisual;
};

// An icon-sized select trigger whose list is presented through the application's
// existing OverlayHost. The host must outlive the Dropdown.
class Dropdown : public Box {
public:
    ~Dropdown() override;

    OverlayHost*          Host{nullptr};
    Render::IFontBackend* FontBackend{nullptr};
    Render::FontHandle    Font{0};
    std::vector<DropdownItem> Items;
    std::size_t           SelectedIndex{0};
    std::function<void(std::size_t)> OnSelectionChanged;

    void ApplyStyle(const DropdownStyle& Style);
    bool GetIsOpen() const;
    void Dismiss();

    bool UpdateInteractionState(const Platform::InputState&) override;

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;

private:
    void Open();

    DropdownStyle                  DropdownVisualStyle{};
    std::shared_ptr<DropdownState> State;
    bool                           PressedInside{false};
};

} // namespace Penumbra::Widgets
