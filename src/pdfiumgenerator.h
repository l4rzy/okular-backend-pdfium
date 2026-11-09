// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <okular/core/generator.h>
#include <cpp/fpdf_scopers.h>
#include <QByteArray>
#include <QImage>
#include <memory>

class PdfiumGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)

public:
    explicit PdfiumGenerator(QObject *parent = nullptr, const QVariantList &args = {});
    ~PdfiumGenerator() override;

    bool loadDocument(const QString &fileName, QList<Okular::Page *> &pages) override;
    PageSizeMetric pagesSizeMetric() const override { return Points; }
    const Okular::DocumentSynopsis *generateDocumentSynopsis() override;
    Okular::DocumentInfo generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const override;

protected:
    bool doCloseDocument() override;
    QImage image(Okular::PixmapRequest *request) override;
    Okular::TextPage *textPage(Okular::TextRequest *request) override;

private:
    // PDFium borrows this buffer until FPDF_CloseDocument().
    QByteArray m_data;
    ScopedFPDFDocument m_document;
    std::unique_ptr<Okular::DocumentSynopsis> m_synopsis;
};
