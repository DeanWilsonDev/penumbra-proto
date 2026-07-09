#include "Penumbra/Widgets/Label.h"

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

} // namespace Penumbra::Widgets
