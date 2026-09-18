#pragma once

#include "Penumbra/Platform/IClipboard.h"
#include "Penumbra/Render/IFontBackend.h"
#include "Penumbra/Render/TextWrap.h"
#include "Penumbra/Widgets/Box.h"
#include "Penumbra/Widgets/FocusState.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Penumbra::Widgets {

// Multi-line text entry. Fixed content size (PreferredWidthLogical x
// PreferredHeightLogical, both demo-supplied, same convention as TextInput's
// PreferredWidthLogical) with content that scrolls vertically past that size --
// a separate class rather than an extension of TextInput because TextInput's
// caret/selection machinery (CaretIndexAtX, TextWidthTo) is written assuming a
// single line, and because a fixed-viewport-with-internal-scroll widget is a
// different shape than TextInput's grow-to-fit single line. Reuses
// ScrollablePanel's clip+offset idea but keeps its own offset rather than
// composing a ScrollablePanel, since scrolling here must stay coupled to caret
// position (scroll-into-view on every caret move) rather than to child widgets.
//
// Word-wraps at the last space that fits (falling back to a hard character
// break when a single word is wider than the field); an explicit '\n' in Text
// always starts a new line. Caret navigation adds Up/Down (sticky column) to
// TextInput's left/right/home/end, with Home/End moving to the start/end of
// the current visual line rather than the whole text. Enter inserts '\n'.
class TextArea : public Box {
public:
    Render::IFontBackend* FontBackend{nullptr};
    Render::FontHandle    Font{0};
    Render::Color         ColorText{0, 0, 0, 0};
    Render::Color         ColorCaret{0, 0, 0, 0};
    Render::Color         ColorSelection{0, 0, 0, 0};
    Render::Color         ColorScrollbarThumb{0, 0, 0, 0};
    float                 CaretWidthLogical{0.0f};      // demo-supplied
    float                 PreferredWidthLogical{0.0f};  // demo-supplied field width
    float                 PreferredHeightLogical{0.0f}; // demo-supplied field height
    float                 WheelStepLogical{0.0f};       // demo-supplied scroll speed; 0 = no opinion
    float                 ScrollbarWidthLogical{0.0f};  // demo-supplied gutter reserved at the right
                                                         // edge for a scroll indicator; 0 = none (text
                                                         // wraps to the full content width and no
                                                         // thumb is drawn, even when content overflows)

    FocusState*           Focus{nullptr};
    Platform::IClipboard* Clipboard{nullptr};
    std::string           Text;

    std::function<void(const std::string&)> OnTextChanged;

    bool UpdateInteractionState(const Platform::InputState&) override;
    bool ConsumedWheelThisFrame() const override { return WheelHandledThisFrame; }

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;

private:
    // One visual (wrapped) line: the rendered slice is Text[ContentStart, ContentEnd).
    // Ownership of a caret index -- which line a given index belongs to -- extends
    // to the next line's ContentStart, so a byte "consumed" by wrapping (the space
    // a soft break drops, or the '\n' a hard break drops) still resolves to the end
    // of this line rather than being ambiguous. Render::TextLine now (the wrap
    // algorithm itself moved to Render::WrapText, shared with Label's own Wrap mode)
    // -- same two fields, aliased rather than redeclared so every existing
    // Line::ContentStart/ContentEnd use below is untouched.
    using Line = Render::TextLine;

    bool IsFocused() const { return Focus != nullptr && Focus->Focused == this; }

    bool        HasSelection() const { return SelectionAnchor != CaretIndex; }
    std::size_t SelectionStart() const { return std::min(SelectionAnchor, CaretIndex); }
    std::size_t SelectionEnd() const { return std::max(SelectionAnchor, CaretIndex); }
    void        DeleteSelection();

    float LineHeight() const;
    float WrapWidthFor(Rect ContentRect) const; // ContentRect.W, minus the scrollbar gutter if any
    void  RebuildLinesIfNeeded(float WrapWidthLogical);
    std::size_t LineIndexForCaret(std::size_t Index) const;
    std::size_t LineOwnershipEnd(std::size_t LineIndex) const; // exclusive; next line's ContentStart, or Text.size()
    float       LineLocalX(const Line&, std::size_t Index) const;   // x offset of Index within its line
    std::size_t IndexAtLineX(const Line&, float LocalX) const;      // nearest caret index for an x offset
    std::size_t IndexAtLocalPoint(float LocalX, float LocalY) const; // click/drag hit-test, in content-local + scroll space

    void MoveCaretVertically(int LineDelta, bool Shift);
    void ScrollCaretIntoView(float ViewportHeightLogical);

    std::size_t CaretIndex{0};
    std::size_t SelectionAnchor{0}; // == CaretIndex means no selection
    bool        Dragging{false};

    std::vector<Line> Lines{{0, 0}};
    float              LinesWrapWidth{-1.0f};
    bool               LinesDirty{true};

    float DesiredCaretXLogical{-1.0f}; // sticky column for consecutive Up/Down; -1 = recompute
    float ScrollOffsetY{0.0f};
    bool  WheelHandledThisFrame{false};
};

} // namespace Penumbra::Widgets
