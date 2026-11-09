// SPDX-License-Identifier: GPL-2.0-or-later
#include "text.h"
#include "pdfiumhelpers.h"

#include <cpp/fpdf_scopers.h>
#include <fpdf_text.h>

namespace {
struct Glyph {
    QString text;
    Okular::NormalizedRect area;
};

void joinEdges(double &a1, double &a2, double &b1, double &b2)
{
    if (a1 + a2 > b1 + b2) {
        joinEdges(b1, b2, a1, a2);
    } else if (a1 + a2 < b1 + b2) {
        const double edge = (a2 + b1) / 2;
        if (edge > a1 && edge < b2) {
            a2 = b1 = edge;
        }
    }
}

// Okular constructs words only when adjacent character edges touch. Partition
// kerning overlaps and character-spacing gaps without crossing word breaks.
void joinCharacterEdges(Okular::NormalizedRect &a, Okular::NormalizedRect &b)
{
    const double overlapY = std::min(a.bottom, b.bottom) - std::max(a.top, b.top);
    const double overlapX = std::min(a.right, b.right) - std::max(a.left, b.left);
    if (overlapY >= 0.6 * std::min(a.bottom - a.top, b.bottom - b.top)) {
        joinEdges(a.left, a.right, b.left, b.right);
    } else if (overlapX >= 0.6 * std::min(a.right - a.left, b.right - b.left)) {
        joinEdges(a.top, a.bottom, b.top, b.bottom);
    }
}
}

std::unique_ptr<Okular::TextPage> pdfiumTextPage(FPDF_PAGE page, Okular::TextRequest *request)
{
    ScopedFPDFTextPage text(FPDFText_LoadPage(page));
    if (!text || request->shouldAbortExtraction()) {
        return nullptr;
    }
    const int count = FPDFText_CountChars(text.get());
    if (count < 0) {
        return nullptr;
    }
    auto result = std::make_unique<Okular::TextPage>();
    const PdfiumHelpers::PageCoordinates coordinates(page);
    std::optional<Glyph> pending;
    bool joinPrevious = false;
    for (int i = 0; i < count; ++i) {
        if (request->shouldAbortExtraction()) {
            return nullptr;
        }
        const char32_t codepoint = FPDFText_GetUnicode(text.get(), i);
        if ((codepoint == U'\r' || codepoint == U'\n') && pending
            && !pending->text.endsWith(QLatin1Char('\n'))) {
            pending->text += QLatin1Char('\n');
        }
        if (codepoint < U' ' || codepoint > 0x10ffff
            || (codepoint >= 0xd800 && codepoint <= 0xdfff)) {
            joinPrevious = false;
            continue;
        }
        // Layout bounds include character advances and font metrics; ink
        // bounds cause Okular to insert spaces between individual letters.
        FS_RECTF box{};
        const auto area = FPDFText_GetLooseCharBox(text.get(), i, &box)
            ? coordinates.rect(box) : std::nullopt;
        if (!area) {
            joinPrevious = false;
            continue;
        }
        Glyph glyph{QString::fromUcs4(&codepoint, 1), *area};
        const bool space = glyph.text.at(0).isSpace();
        if (pending) {
            if (joinPrevious && !space) {
                joinCharacterEdges(pending->area, glyph.area);
            }
            result->append(pending->text, pending->area);
        }
        pending = std::move(glyph);
        joinPrevious = !space;
    }
    if (pending) {
        result->append(pending->text, pending->area);
    }
    return request->shouldAbortExtraction() ? nullptr : std::move(result);
}
