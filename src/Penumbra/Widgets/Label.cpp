#include "Penumbra/Widgets/Label.h"

#include <string_view>
#include <utility>

namespace Penumbra::Widgets {

float Label::LineHeight() const {
    return FontBackend ? FontBackend->MeasureText(Font, "Ag").HeightLogical : 0.0f;
}

Point Label::MeasureContent(Point AvailableContentSize) {
    if (!FontBackend) {
        return {0.0f, 0.0f};
    }

    if (Wrap) {
        const std::vector<Render::TextLine> WrappedLines =
            Render::WrapText(FontBackend, Font, Text, AvailableContentSize.X);
        // Reports the full available width (a block of wrapped text fills its
        // container, like CSS `white-space: normal` text does) rather than the
        // widest actual line -- critical so Arrange's own ChildExtent (Box::
        // PlaceCross, CrossAlign::Start included) ends up equal to the same width
        // DrawContent re-wraps against below; a narrower reported width here would
        // let Arrange shrink this Label below the width its own line count/height
        // was computed against, reflowing to fewer, longer lines at Draw time than
        // Measure sized for. Falls back to the widest actual line only when there's
        // no usable width to wrap against yet (AvailableContentSize.X <= 0), so an
        // as-yet-unconstrained Label doesn't collapse to a zero-width, all-lines
        // report.
        float ReportedWidth = AvailableContentSize.X;
        if (ReportedWidth <= 0.0f) {
            for (const Render::TextLine& L : WrappedLines) {
                ReportedWidth = std::max(
                    ReportedWidth,
                    FontBackend->MeasureTextWidth(Font, std::string_view(Text).substr(L.ContentStart, L.ContentEnd - L.ContentStart)));
            }
        }
        return {ReportedWidth, LineHeight() * static_cast<float>(WrappedLines.size())};
    }

    const Render::TextMetrics Metrics = FontBackend->MeasureText(Font, Text);
    float Width = Metrics.WidthLogical;
    // Clamped here (not just at Draw time) so a truncating Label doesn't blow
    // out sibling layout by reporting its full unbounded intrinsic width --
    // the same reasoning IconWidget's own fixed SizeLogical exists for, just
    // conditional on MaxWidthLogical being set at all.
    if (MaxWidthLogical && Width > *MaxWidthLogical) {
        Width = *MaxWidthLogical;
    }
    return {Width, Metrics.HeightLogical};
}

void Label::DrawContent(Render::Renderer& Renderer, Rect ContentRect) {
    if (Wrap) {
        const std::vector<Render::TextLine> WrappedLines = Render::WrapText(FontBackend, Font, Text, ContentRect.W);
        const float LH = LineHeight();
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
