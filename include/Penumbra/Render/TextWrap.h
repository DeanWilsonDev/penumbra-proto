#pragma once

#include "Penumbra/Render/IFontBackend.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Penumbra::Render {

// One visual (wrapped) line within a larger string: the rendered slice is
// Text[ContentStart, ContentEnd). Shared by TextArea (which additionally tracks
// caret/selection state per line, see its own Line struct) and Label (Wrap ==
// true, no editing state at all) so both widgets word-wrap identically instead
// of each carrying its own copy of the algorithm.
struct TextLine {
    std::size_t ContentStart{0};
    std::size_t ContentEnd{0};
};

// Greedy word-wrap, ported unchanged from TextArea's own former
// RebuildLinesIfNeeded: breaks at the last space that fits within
// WrapWidthLogical, falling back to a hard character break when a single word
// alone is wider than WrapWidthLogical. An explicit '\n' in Text always starts
// a new line. Always returns at least one line, even for empty Text. No
// FontBackend, or WrapWidthLogical <= 0.0f, falls back to hard-newline-only
// splitting (no width-based wrapping at all) -- "can't measure, don't guess."
std::vector<TextLine> WrapText(IFontBackend* FontBackend, FontHandle Font, const std::string& Text,
                                float WrapWidthLogical);

} // namespace Penumbra::Render
