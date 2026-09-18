#include "Penumbra/Render/TextWrap.h"

#include <string_view>

namespace Penumbra::Render {

std::vector<TextLine> WrapText(IFontBackend* FontBackend, FontHandle Font, const std::string& Text,
                                float WrapWidthLogical) {
    std::vector<TextLine> Lines;
    const std::size_t N = Text.size();

    if (!FontBackend || WrapWidthLogical <= 0.0f) {
        // No font or no room to measure against: fall back to hard-newline-only lines.
        std::size_t LineStart = 0;
        for (std::size_t Index = 0; Index <= N; ++Index) {
            if (Index == N || Text[Index] == '\n') {
                Lines.push_back({LineStart, Index});
                LineStart = Index + 1;
            }
        }
    } else {
        std::size_t LineStart = 0;
        std::size_t Cursor = 0;
        while (Cursor <= N) {
            if (Cursor == N || Text[Cursor] == '\n') {
                Lines.push_back({LineStart, Cursor});
                if (Cursor == N) {
                    break;
                }
                LineStart = Cursor + 1;
                Cursor = LineStart;
                continue;
            }

            const float Width = FontBackend->MeasureTextWidth(
                Font, std::string_view(Text).substr(LineStart, Cursor + 1 - LineStart));
            if (Width > WrapWidthLogical && Cursor > LineStart) {
                const std::size_t SpaceAt = Text.rfind(' ', Cursor - 1);
                if (SpaceAt != std::string::npos && SpaceAt >= LineStart) {
                    // Word wrap: drop the space the break lands on.
                    Lines.push_back({LineStart, SpaceAt});
                    LineStart = SpaceAt + 1;
                } else {
                    // A single word is wider than the field: hard character break.
                    Lines.push_back({LineStart, Cursor});
                    LineStart = Cursor;
                }
                Cursor = LineStart;
                continue;
            }
            ++Cursor;
        }
    }

    if (Lines.empty()) {
        Lines.push_back({0, 0});
    }

    return Lines;
}

} // namespace Penumbra::Render
