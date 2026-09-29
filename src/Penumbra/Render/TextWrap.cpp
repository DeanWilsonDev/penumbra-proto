#include "Penumbra/Render/TextWrap.h"

#include <string_view>

namespace Penumbra::Render {

std::vector<TextLine> WrapText(IFontBackend* FontBackend, FontHandle Font, const std::string& Text,
                                float WrapWidthLogical) {
    std::vector<TextLine> Lines;
    const std::size_t N = Text.size();

    if (!FontBackend || WrapWidthLogical <= 0.0f) {
        std::size_t LineStart = 0;
        for (std::size_t Index = 0; Index <= N; ++Index) {
            if (Index == N || Text[Index] == '\n') {
                Lines.push_back({LineStart, Index});
                LineStart = Index + 1;
            }
        }
    } else {
        const std::string_view View(Text);
        std::size_t LineStart = 0;
        const auto Overflows = [&](std::size_t Cursor) {
            return FontBackend->MeasureTextWidth(Font, View.substr(LineStart, Cursor + 1 - LineStart)) >
                   WrapWidthLogical;
        };

        while (true) {
            const std::size_t NewlineAt  = Text.find('\n', LineStart);
            const std::size_t SegmentEnd = NewlineAt == std::string::npos ? N : NewlineAt;

            while (true) {
                std::size_t Fits = LineStart;
                std::size_t Over = std::string::npos;
                for (std::size_t Probe = LineStart + 1; Probe < SegmentEnd;) {
                    const std::size_t NextSpace = Text.find(' ', Probe);
                    const std::size_t Candidate = NextSpace < SegmentEnd ? NextSpace : SegmentEnd - 1;
                    if (Overflows(Candidate)) {
                        Over = Candidate;
                        break;
                    }
                    Fits  = Candidate;
                    Probe = Candidate + 1;
                }
                if (Over == std::string::npos) {
                    break;
                }

                if (Fits > LineStart || Text[LineStart] == ' ') {
                    Lines.push_back({LineStart, Fits});
                    LineStart = Fits + 1;
                } else {
                    std::size_t Cursor = LineStart + 1;
                    while (Cursor < Over && !Overflows(Cursor)) {
                        ++Cursor;
                    }
                    Lines.push_back({LineStart, Cursor});
                    LineStart = Cursor;
                }
            }

            Lines.push_back({LineStart, SegmentEnd});
            if (SegmentEnd == N) {
                break;
            }
            LineStart = SegmentEnd + 1;
        }
    }

    if (Lines.empty()) {
        Lines.push_back({0, 0});
    }

    return Lines;
}

} // namespace Penumbra::Render
