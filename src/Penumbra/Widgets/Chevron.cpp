#include "Penumbra/Widgets/Chevron.h"

namespace Penumbra::Widgets {

Point Chevron::MeasureContent(Point) { return ContentSizeLogical; }

void Chevron::DrawContent(Render::Renderer& Renderer, Rect ContentRect) {
    const Point Center{ContentRect.X + ContentRect.W * 0.5f, ContentRect.Y + ContentRect.H * 0.5f};
    if (Direction == ChevronDirection::Down) {
        Renderer.DrawLine({Center.X - RadiusLogical, Center.Y - RadiusLogical * 0.3f},
                          {Center.X, Center.Y + RadiusLogical * 0.5f}, Color, ThicknessLogical);
        Renderer.DrawLine({Center.X, Center.Y + RadiusLogical * 0.5f},
                          {Center.X + RadiusLogical, Center.Y - RadiusLogical * 0.3f}, Color, ThicknessLogical);
        return;
    }
    Renderer.DrawLine({Center.X - RadiusLogical * 0.3f, Center.Y - RadiusLogical},
                      {Center.X + RadiusLogical * 0.5f, Center.Y}, Color, ThicknessLogical);
    Renderer.DrawLine({Center.X + RadiusLogical * 0.5f, Center.Y},
                      {Center.X - RadiusLogical * 0.3f, Center.Y + RadiusLogical}, Color, ThicknessLogical);
}

} // namespace Penumbra::Widgets
