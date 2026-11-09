// SPDX-License-Identifier: GPL-2.0-or-later
#include "navigation.h"
#include "pdfiumhelpers.h"

#include <okular/core/action.h>
#include <fpdf_doc.h>
#include <cpp/fpdf_scopers.h>
#include <QSet>
#include <QUrl>
#include <limits>

namespace {
Okular::DocumentViewport destination(FPDF_DOCUMENT document, FPDF_DEST dest)
{
    const int index = dest ? FPDFDest_GetDestPageIndex(document, dest) : -1;
    if (index < 0 || index >= FPDF_GetPageCount(document)) {
        return Okular::DocumentViewport();
    }
    Okular::DocumentViewport viewport(index);
    ScopedFPDFPage page(FPDF_LoadPage(document, index));
    if (!page) {
        return viewport;
    }
    const PdfiumHelpers::PageCoordinates coordinates(page.get());
    const auto topLeft = coordinates.pageTopLeft();
    FPDF_BOOL hasX = false, hasY = false, hasZoom = false;
    float dx = 0, dy = 0, zoom = 0;
    FPDFDest_GetLocationInPage(dest, &hasX, &hasY, &hasZoom, &dx, &dy, &zoom);
    unsigned long count = 0;
    float params[4]{};
    const auto view = FPDFDest_GetView(dest, &count, params);
    if ((view == PDFDEST_VIEW_FITH || view == PDFDEST_VIEW_FITBH) && count >= 1) {
        hasY = true;
        dy = params[0];
    } else if ((view == PDFDEST_VIEW_FITV || view == PDFDEST_VIEW_FITBV) && count >= 1) {
        hasX = true;
        dx = params[0];
    } else if (view == PDFDEST_VIEW_FITR && count >= 4) {
        hasX = hasY = true;
        dx = params[0];
        dy = params[3];
    }
    if (hasX || hasY) {
        const auto point = coordinates.point(hasX ? dx : topLeft.x(), hasY ? dy : topLeft.y());
        if (point) {
            viewport.rePos.normalizedX = point->x();
            viewport.rePos.normalizedY = point->y();
            viewport.rePos.enabled = true;
            viewport.rePos.pos = Okular::DocumentViewport::TopLeft;
        }
    }
    return viewport;
}

std::unique_ptr<Okular::Action> action(FPDF_DOCUMENT document, FPDF_DEST dest, FPDF_ACTION pdfAction)
{
    if (!dest && pdfAction && FPDFAction_GetType(pdfAction) == PDFACTION_GOTO) {
        dest = FPDFAction_GetDest(document, pdfAction);
    }
    if (dest) {
        auto viewport = destination(document, dest);
        if (viewport.isValid()) {
            return std::make_unique<Okular::GotoAction>(QString(), viewport);
        }
    } else if (pdfAction && FPDFAction_GetType(pdfAction) == PDFACTION_URI) {
        const auto size = FPDFAction_GetURIPath(document, pdfAction, nullptr, 0);
        if (size > 1 && size <= std::numeric_limits<int>::max()) {
            QByteArray uri(static_cast<int>(size), '\0');
            if (FPDFAction_GetURIPath(document, pdfAction, uri.data(), size) == size) {
                uri.chop(1);
                const QUrl url = QUrl::fromEncoded(uri);
                if (url.isValid() && !url.isEmpty()) {
                    return std::make_unique<Okular::BrowseAction>(url);
                }
            }
        }
    }
    return nullptr;
}

QString bookmarkTitle(FPDF_BOOKMARK bookmark)
{
    return PdfiumHelpers::readUtf16([bookmark](void *buffer, unsigned long size) {
        return FPDFBookmark_GetTitle(bookmark, buffer, size);
    });
}
}

QList<Okular::ObjectRect *> PdfiumNavigation::links(FPDF_DOCUMENT document, int pageNumber)
{
    QList<Okular::ObjectRect *> result;
    ScopedFPDFPage page(FPDF_LoadPage(document, pageNumber));
    if (!page) {
        return result;
    }
    const PdfiumHelpers::PageCoordinates coordinates(page.get());
    int position = 0;
    FPDF_LINK link = nullptr;
    while (FPDFLink_Enumerate(page.get(), &position, &link)) {
        FS_RECTF rect{};
        const auto area = FPDFLink_GetAnnotRect(link, &rect) ? coordinates.rect(rect) : std::nullopt;
        if (!area) {
            continue;
        }
        auto target = action(document, FPDFLink_GetDest(document, link), FPDFLink_GetAction(link));
        if (target) {
            result.append(new Okular::ObjectRect(*area, false, Okular::ObjectRect::Action, target.release()));
        }
    }
    return result;
}

std::unique_ptr<Okular::DocumentSynopsis> PdfiumNavigation::synopsis(FPDF_DOCUMENT document)
{
    auto first = FPDFBookmark_GetFirstChild(document, nullptr);
    if (!first) {
        return nullptr;
    }
    auto result = std::make_unique<Okular::DocumentSynopsis>();
    QSet<FPDF_BOOKMARK> visited;
    const auto append = [&](auto &&self, FPDF_BOOKMARK bookmark, QDomNode parent, int depth) -> void {
        // Malformed outlines can contain cycles or excessive nesting.
        if (depth > 128) {
            return;
        }
        for (; bookmark && !visited.contains(bookmark);
             bookmark = FPDFBookmark_GetNextSibling(document, bookmark)) {
            visited.insert(bookmark);
            QDomElement element = result->createElement(bookmarkTitle(bookmark));
            element.setAttribute(QStringLiteral("Open"), FPDFBookmark_GetCount(bookmark) > 0 ? QStringLiteral("true") : QStringLiteral("false"));
            auto target = action(document, FPDFBookmark_GetDest(document, bookmark), FPDFBookmark_GetAction(bookmark));
            if (target && target->actionType() == Okular::Action::Goto) {
                element.setAttribute(QStringLiteral("Viewport"), static_cast<Okular::GotoAction *>(target.get())->destViewport().toString());
            } else if (target && target->actionType() == Okular::Action::Browse) {
                element.setAttribute(QStringLiteral("URL"), static_cast<Okular::BrowseAction *>(target.get())->url().toString());
            }
            self(self, FPDFBookmark_GetFirstChild(document, bookmark), element, depth + 1);
            parent.appendChild(element);
        }
    };
    append(append, first, *result, 0);
    return result;
}
