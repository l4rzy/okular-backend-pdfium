// SPDX-License-Identifier: GPL-2.0-or-later
#include "pdfiumgenerator.h"
#include "metadata.h"
#include "navigation.h"
#include "text.h"

#include <okular/core/page.h>
#include <fpdf_progressive.h>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QScopeGuard>
#include <cmath>
#include <limits>
#include <vector>

K_PLUGIN_CLASS_WITH_JSON(PdfiumGenerator, "manifest.json")

namespace {
// PDFium is not thread-safe, including across different documents.
QMutex pdfiumMutex;
int users = 0;
}

PdfiumGenerator::PdfiumGenerator(QObject *parent, const QVariantList &args)
    : Okular::Generator(parent, args)
{
    QMutexLocker lock(&pdfiumMutex);
    if (users++ == 0) {
        FPDF_LIBRARY_CONFIG config{};
        config.version = 2;
        FPDF_InitLibraryWithConfig(&config);
    }
    setFeature(Threaded);
    setFeature(TextExtraction);
    setFeature(TiledRendering);
    setFeature(SupportsCancelling);
}

PdfiumGenerator::~PdfiumGenerator()
{
    // Wait for Okular's render thread before releasing the document and engine.
    closeDocument();
    QMutexLocker lock(&pdfiumMutex);
    if (--users == 0) {
        FPDF_DestroyLibrary();
    }
}

bool PdfiumGenerator::loadDocument(const QString &fileName, QList<Okular::Page *> &pages)
{
    // Stop active work before replacing the borrowed input buffer.
    if (!pages.isEmpty() || !closeDocument()) {
        return false;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError || data.isEmpty()) {
        return false;
    }

    QMutexLocker lock(&pdfiumMutex);
    ScopedFPDFDocument document(FPDF_LoadMemDocument64(data.constData(), static_cast<size_t>(data.size()), nullptr));
    const int count = document ? FPDF_GetPageCount(document.get()) : 0;
    if (count <= 0) {
        return false;
    }
    std::vector<std::unique_ptr<Okular::Page>> ownedPages;
    QList<Okular::Page *> loadedPages;
    for (int i = 0; i < count; ++i) {
        FS_SIZEF size{};
        if (!FPDF_GetPageSizeByIndexF(document.get(), i, &size)
            || !std::isfinite(size.width) || !std::isfinite(size.height)
            || size.width <= 0 || size.height <= 0) {
            return false;
        }
        auto page = std::make_unique<Okular::Page>(i, size.width, size.height, Okular::Rotation0);
        page->setObjectRects(PdfiumNavigation::links(document.get(), i));
        ownedPages.push_back(std::move(page));
        loadedPages.append(ownedPages.back().get());
    }
    m_data = std::move(data);
    m_document = std::move(document);
    pages = std::move(loadedPages);
    for (auto &page : ownedPages) {
        page.release(); // Okular owns the pages after a successful load.
    }
    return true;
}

bool PdfiumGenerator::doCloseDocument()
{
    QMutexLocker lock(&pdfiumMutex);
    m_synopsis.reset();
    m_document.reset();
    m_data.clear();
    return true;
}

Okular::DocumentInfo PdfiumGenerator::generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const
{
    QMutexLocker lock(&pdfiumMutex);
    return pdfiumDocumentInfo(m_document.get(), keys);
}

const Okular::DocumentSynopsis *PdfiumGenerator::generateDocumentSynopsis()
{
    QMutexLocker lock(&pdfiumMutex);
    if (m_document && !m_synopsis) {
        m_synopsis = PdfiumNavigation::synopsis(m_document.get());
    }
    return m_synopsis.get();
}

QImage PdfiumGenerator::image(Okular::PixmapRequest *request)
{
    if (!request || request->shouldAbortRender() || request->width() <= 0 || request->height() <= 0) {
        return {};
    }
    QRect region(0, 0, request->width(), request->height());
    if (request->isTile()) {
        const auto &rect = request->normalizedRect();
        if (!std::isfinite(rect.left) || !std::isfinite(rect.top)
            || !std::isfinite(rect.right) || !std::isfinite(rect.bottom)
            || rect.left < 0 || rect.top < 0 || rect.right > 1 || rect.bottom > 1
            || rect.left >= rect.right || rect.top >= rect.bottom
            || request->width() == std::numeric_limits<int>::max()
            || request->height() == std::numeric_limits<int>::max()) {
            return {};
        }
        // Match Okular's inclusive right/bottom pixel edges, including the
        // extra white edge pixel for tiles that reach the page boundary.
        region = rect.geometry(request->width(), request->height());
    }
    if (region.width() > std::numeric_limits<int>::max() / 4) {
        return {};
    }
    QMutexLocker lock(&pdfiumMutex);
    if (request->shouldAbortRender() || !m_document || request->pageNumber() < 0
        || request->pageNumber() >= FPDF_GetPageCount(m_document.get())) {
        return {};
    }
    ScopedFPDFPage page(FPDF_LoadPage(m_document.get(), request->pageNumber()));
    if (!page || request->shouldAbortRender()) {
        return {};
    }
    // RGBA with REVERSE_BYTE_ORDER also works on big-endian Qt platforms.
    QImage result(region.size(), QImage::Format_RGBA8888);
    if (result.isNull()) {
        return {};
    }
    result.fill(Qt::white);
    ScopedFPDFBitmap bitmap(FPDFBitmap_CreateEx(result.width(), result.height(),
        FPDFBitmap_BGRA, result.bits(), static_cast<int>(result.bytesPerLine())));
    if (!bitmap || request->shouldAbortRender()) {
        return {};
    }
    IFSDK_PAUSE pause{};
    pause.version = 1;
    pause.user = request;
    pause.NeedToPauseNow = [](IFSDK_PAUSE *callback) -> FPDF_BOOL {
        return static_cast<Okular::PixmapRequest *>(callback->user)->shouldAbortRender();
    };
    // Destroy progressive state before the bitmap and its QImage buffer,
    // including when rendering fails or a request is cancelled.
    const auto closeRender = qScopeGuard([&] { FPDF_RenderPage_Close(page.get()); });
    int status = FPDF_RenderPageBitmap_Start(bitmap.get(), page.get(), -region.x(), -region.y(),
        request->width(), request->height(), 0, FPDF_ANNOT | FPDF_REVERSE_BYTE_ORDER, &pause);
    while (status == FPDF_RENDER_TOBECONTINUED && !request->shouldAbortRender()) {
        status = FPDF_RenderPage_Continue(page.get(), &pause);
    }
    return status == FPDF_RENDER_DONE && !request->shouldAbortRender() ? result : QImage();
}

Okular::TextPage *PdfiumGenerator::textPage(Okular::TextRequest *request)
{
    if (!request || request->shouldAbortExtraction() || !request->page()) {
        return nullptr;
    }
    QMutexLocker lock(&pdfiumMutex);
    const int index = request->page()->number();
    if (request->shouldAbortExtraction() || !m_document || index < 0
        || index >= FPDF_GetPageCount(m_document.get())) {
        return nullptr;
    }
    ScopedFPDFPage page(FPDF_LoadPage(m_document.get(), index));
    return page ? pdfiumTextPage(page.get(), request).release() : nullptr;
}

#include "pdfiumgenerator.moc"
