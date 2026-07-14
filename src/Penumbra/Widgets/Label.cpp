#include "Penumbra/Widgets/Label.h"

#include <utility>

namespace Penumbra::Widgets {

Point Label::MeasureContent(Point /*AvailableContentSize*/) {
    if (!FontBackend) {
        return {0.0f, 0.0f};
    }
    const Render::TextMetrics Metrics = FontBackend->MeasureText(Font, Text);
    return {Metrics.WidthLogical, Metrics.HeightLogical};
}

void Label::DrawContent(Render::Renderer& Renderer, Rect ContentRect) {
    Renderer.DrawText(Font, Text, {ContentRect.X, ContentRect.Y}, ColorText);
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
