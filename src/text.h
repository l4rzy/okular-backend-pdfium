// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <okular/core/generator.h>
#include <okular/core/textpage.h>
#include <fpdfview.h>
#include <memory>

// Call with the generator's PDFium mutex held and a live page/request.
std::unique_ptr<Okular::TextPage> pdfiumTextPage(FPDF_PAGE page, Okular::TextRequest *request);
