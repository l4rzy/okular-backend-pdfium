// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <okular/core/document.h>
#include <fpdfview.h>

// Call with the generator's PDFium mutex held.
Okular::DocumentInfo pdfiumDocumentInfo(FPDF_DOCUMENT document,
    const QSet<Okular::DocumentInfo::Key> &keys);
