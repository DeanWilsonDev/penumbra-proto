#include "Penumbra/Widgets/Length.h"

namespace Penumbra::Widgets {

namespace {
Point LayoutViewportLogical{0.0f, 0.0f};
} // namespace

void SetLayoutViewportLogical(Point SizeLogical) { LayoutViewportLogical = SizeLogical; }

Point GetLayoutViewportLogical() { return LayoutViewportLogical; }

float Length::Resolve(float PercentBasisLogical) const {
    switch (Unit) {
    case LengthUnit::Logical:
        return Value;
    case LengthUnit::Percent:
        return PercentBasisLogical * Value / 100.0f;
    case LengthUnit::ViewportWidth:
        return LayoutViewportLogical.X * Value / 100.0f;
    case LengthUnit::ViewportHeight:
        return LayoutViewportLogical.Y * Value / 100.0f;
    }
    return Value;
}

} // namespace Penumbra::Widgets
