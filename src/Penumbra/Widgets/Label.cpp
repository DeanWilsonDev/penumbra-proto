#include "Penumbra/Widgets/Label.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace Penumbra::Widgets {

float Label::LineHeight() const {
    return FontBackend ? FontBackend->MeasureText(Font, "Ag").HeightLogical : 0.0f;
}

const std::vector<Render::TextLine>& Label::WrappedLinesFor(float WidthLogical) {
    if (WrapWidth != WidthLogical || WrapFontBackend != FontBackend || WrapFont != Font || WrappedText != Text) {
        const float Tolerance = WidthLogical > 0.0f ? 0.01f : 0.0f;
        WrapLines       = Render::WrapText(FontBackend, Font, Text, WidthLogical + Tolerance);
        WrapLineHeight  = LineHeight();
        WrapWidth       = WidthLogical;
        WrapFontBackend = FontBackend;
        WrapFont        = Font;
        WrappedText     = Text;
    }
    return WrapLines;
}

Point Label::MeasureContent(Point AvailableContentSize) {
    if (!FontBackend) {
        return {0.0f, 0.0f};
    }

    if (Wrap) {
        const std::vector<Render::TextLine>& WrappedLines = WrappedLinesFor(AvailableContentSize.X);
        const std::string_view               View(Text);
        float                                Widest = 0.0f;
        for (const Render::TextLine& L : WrappedLines) {
            const std::size_t End = (L.ContentEnd < View.size() && View[L.ContentEnd] == ' ') ? L.ContentEnd + 1 : L.ContentEnd;
            Widest = std::max(Widest, FontBackend->MeasureTextWidth(Font, View.substr(L.ContentStart, End - L.ContentStart)));
        }
        const float ReportedWidth = AvailableContentSize.X > 0.0f ? std::min(Widest, AvailableContentSize.X) : Widest;
        return {ReportedWidth, WrapLineHeight * static_cast<float>(WrappedLines.size())};
    }

    const Render::TextMetrics Metrics = FontBackend->MeasureText(Font, Text);
    float Width = Metrics.WidthLogical;
    if (MaxWidthLogical && Width > *MaxWidthLogical) {
        Width = *MaxWidthLogical;
    }
    return {Width, Metrics.HeightLogical};
}

void Label::DrawContent(Render::Renderer& Renderer, Rect ContentRect) {
    if (Wrap) {
        const std::vector<Render::TextLine>& WrappedLines = WrappedLinesFor(ContentRect.W);
        const float LH = WrapLineHeight;
        for (std::size_t Index = 0; Index < WrappedLines.size(); ++Index) {
            const Render::TextLine& L = WrappedLines[Index];
            if (L.ContentEnd > L.ContentStart) {
                Renderer.DrawText(Font, std::string_view(Text).substr(L.ContentStart, L.ContentEnd - L.ContentStart),
                                   {ContentRect.X, ContentRect.Y + static_cast<float>(Index) * LH}, ColorText);
            }
        }
        return;
    }

    if (!MaxWidthLogical || Renderer.MeasureTextWidth(Font, Text) <= *MaxWidthLogical) {
        Renderer.DrawText(Font, Text, {ContentRect.X, ContentRect.Y}, ColorText);
        return;
    }

    if (!TruncateWithEllipsis) {
        // `text-overflow: clip` -- draw the full string behind a clip rect
        // rather than shortening it at all.
        Renderer.PushClipRect({ContentRect.X, ContentRect.Y, *MaxWidthLogical, ContentRect.H});
        Renderer.DrawText(Font, Text, {ContentRect.X, ContentRect.Y}, ColorText);
        Renderer.PopClipRect();
        return;
    }

    // `text-overflow: ellipsis` -- shrink one character at a time until what's
    // left fits, then swap the trailing one or two characters for "..". Same
    // algorithm pharos-proto's own drawTruncatedText (ui/text_utils.cpp) used
    // before this existed as a real Label capability.
    std::string ToDraw = Text;
    while (!ToDraw.empty() && Renderer.MeasureTextWidth(Font, ToDraw) > *MaxWidthLogical) {
        ToDraw.pop_back();
    }
    if (ToDraw.size() >= 2) {
        ToDraw[ToDraw.size() - 1] = '.';
        if (ToDraw.size() >= 3) {
            ToDraw[ToDraw.size() - 2] = '.';
        }
    }
    Renderer.DrawText(Font, ToDraw, {ContentRect.X, ContentRect.Y}, ColorText);
}

Label::Builder::Builder() : Owned(std::make_unique<Label>()) {}

Label::Builder& Label::Builder::className(std::string Value) {
    Owned->ClassName = std::move(Value);
    return *this;
}

Label::Builder& Label::Builder::text(std::string Value) {
    Owned->Text = std::move(Value);
    return *this;
}

Label::Builder& Label::Builder::child(std::unique_ptr<WidgetBase> Child) {
    Owned->AddChild(std::move(Child));
    return *this;
}

Label::Builder& Label::Builder::children(std::vector<std::unique_ptr<WidgetBase>> Kids) {
    for (auto& Kid : Kids) {
        Owned->AddChild(std::move(Kid));
    }
    return *this;
}

Label::Builder& Label::Builder::onPress(std::function<void()> Handler) {
    Owned->OnPressed = std::move(Handler);
    return *this;
}

Label::Builder& Label::Builder::onRelease(std::function<void()> Handler) {
    Owned->OnReleased = std::move(Handler);
    return *this;
}

Label::Builder& Label::Builder::onHover(std::function<void()> Handler) {
    Owned->OnHovered = std::move(Handler);
    return *this;
}

Label::Builder& Label::Builder::onFocus(std::function<void()> Handler) {
    Owned->OnFocused = std::move(Handler);
    return *this;
}

Label::Builder& Label::Builder::onChange(std::function<void()> Handler) {
    Owned->OnChanged = std::move(Handler);
    return *this;
}

std::unique_ptr<Label> Label::Builder::build() { return std::move(Owned); }

} // namespace Penumbra::Widgets
