// SPDX-License-Identifier: GPL-2.0-or-later
#include "metadata.h"
#include "pdfiumhelpers.h"

#include <fpdf_doc.h>
#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QTimeZone>

namespace {
QString metadata(FPDF_DOCUMENT document, const char *tag)
{
    return PdfiumHelpers::readUtf16([document, tag](void *buffer, unsigned long size) {
        return FPDF_GetMetaText(document, tag, buffer, size);
    });
}

QString displayDate(const QString &raw)
{
    // PDF dates allow omitted components and UTC offsets such as +05'30'.
    static const QRegularExpression pattern(QStringLiteral(
        "^(?:D:)?([0-9]{4})((?:[0-9]{2}){0,5})(Z|[+-][0-9]{2}(?:'?[0-9]{2})?'?)?$"));
    const auto match = pattern.match(raw);
    if (!match.hasMatch()) {
        return raw;
    }
    const QString parts = match.captured(2);
    const auto part = [&parts](int offset, int fallback) {
        return parts.size() >= offset + 2 ? parts.mid(offset, 2).toInt() : fallback;
    };
    QTimeZone zone = QTimeZone::systemTimeZone();
    QString offset = match.captured(3);
    if (offset == QLatin1String("Z")) {
        zone = QTimeZone::utc();
    } else if (!offset.isEmpty()) {
        offset.remove(QLatin1Char('\''));
        const int hours = offset.mid(1, 2).toInt();
        const int minutes = offset.size() > 3 ? offset.mid(3, 2).toInt() : 0;
        if (hours > 23 || minutes > 59) {
            return raw;
        }
        const int seconds = (hours * 3600 + minutes * 60) * (offset[0] == QLatin1Char('-') ? -1 : 1);
        zone = QTimeZone::fromSecondsAheadOfUtc(seconds);
    }
    const QDate date(match.captured(1).toInt(), part(0, 1), part(2, 1));
    const QTime time(part(4, 0), part(6, 0), part(8, 0));
    const QDateTime value(date, time, zone);
    return value.isValid() ? QLocale().toString(value.toLocalTime(), QLocale::LongFormat) : raw;
}
}

Okular::DocumentInfo pdfiumDocumentInfo(FPDF_DOCUMENT document,
    const QSet<Okular::DocumentInfo::Key> &keys)
{
    Okular::DocumentInfo result;
    if (!document) {
        return result;
    }
    using Info = Okular::DocumentInfo;
    result.set(Info::MimeType, QStringLiteral("application/pdf"));
    result.set(Info::Pages, QString::number(FPDF_GetPageCount(document)));
    const struct { Info::Key key; const char *tag; } fields[] = {
        {Info::Title, "Title"}, {Info::Author, "Author"},
        {Info::Subject, "Subject"}, {Info::Keywords, "Keywords"},
        {Info::Creator, "Creator"}, {Info::Producer, "Producer"},
        {Info::CreationDate, "CreationDate"}, {Info::ModificationDate, "ModDate"},
    };
    for (const auto &field : fields) {
        if (!keys.contains(field.key)) {
            continue;
        }
        const QString value = metadata(document, field.tag);
        if (!value.isEmpty()) {
            const bool date = field.key == Info::CreationDate || field.key == Info::ModificationDate;
            result.set(field.key, date ? displayDate(value) : value);
        }
    }
    if (keys.contains(Info::CustomKeys)) {
        int version = 0;
        if (FPDF_GetFileVersion(document, &version)) {
            result.set(QStringLiteral("format"), QStringLiteral("PDF %1.%2").arg(version / 10).arg(version % 10),
                QObject::tr("Format"));
        }
        result.set(QStringLiteral("encryption"), FPDF_GetSecurityHandlerRevision(document) >= 0
                ? QObject::tr("Encrypted") : QObject::tr("Unencrypted"), QObject::tr("Security"));
    }
    return result;
}
