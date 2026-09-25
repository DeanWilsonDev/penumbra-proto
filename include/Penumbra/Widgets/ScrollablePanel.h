#pragma once

#include "Penumbra/Widgets/Box.h"

namespace Penumbra::Widgets {

class ScrollablePanel : public Box {
public:
    enum class ScrollDirection { Vertical, Horizontal };

    ScrollDirection Direction{ScrollDirection::Vertical};

    float WheelStepLogical{0.0f};
    float HorizontalWheelStepLogical{0.0f};

    float         ScrollbarWidthLogical{0.0f};
    Render::Color ColorScrollbarThumb{0, 0, 0, 0};
    Render::Color ColorScrollbarThumbHovered{0, 0, 0, 0};
    Render::Color ColorScrollbarTrack{0, 0, 0, 0};

    float GetScrollOffset() const { return ScrollOffsetY; }
    float GetScrollOffsetX() const { return ScrollOffsetX; }

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;
    bool  UpdateInteractionState(const Platform::InputState&) override;
    void  Draw(Render::Renderer&) override;

    bool ConsumedWheelThisFrame() const override { return WheelHandledThisFrame; }

private:
    void DrawScrollbar(Render::Renderer&, Rect Content) const;

    float ScrollOffsetY{0.0f};
    float ScrollOffsetX{0.0f};
    float ContentHeight{0.0f};
    float ContentWidth{0.0f};
    bool  WheelHandledThisFrame{false};
    bool  PointerOver{false};
};

} // namespace Penumbra::Widgets
