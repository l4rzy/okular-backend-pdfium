// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <okular/core/document.h>
#include <okular/core/area.h>
#include <fpdfview.h>
#include <memory>

// Call with the generator's PDFium mutex held.
namespace PdfiumNavigation {
QList<Okular::ObjectRect *> links(FPDF_DOCUMENT document, int pageNumber);
std::unique_ptr<Okular::DocumentSynopsis> synopsis(FPDF_DOCUMENT document);
}
