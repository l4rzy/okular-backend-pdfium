// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <okular/core/area.h>
#include <fpdfview.h>
#include <QByteArray>
#include <QString>
#include <QTransform>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace PdfiumHelpers {
// PDFium string APIs return a byte count including the UTF-16LE terminator.
template<typename Read>
QString readUtf16(Read read)
{
    const auto size = read(nullptr, 0);
    if (size <= 2 || size % 2 || size > std::numeric_limits<int>::max()) {
        return {};
    }
    QByteArray bytes(static_cast<int>(size), '\0');
    if (read(bytes.data(), size) != size || bytes[bytes.size() - 1] || bytes[bytes.size() - 2]) {
        return {};
    }
    QString value;
    value.reserve(bytes.size() / 2 - 1);
    for (qsizetype i = 0; i + 2 < bytes.size(); i += 2) {
        value += QChar(qFromLittleEndian<quint16>(bytes.constData() + i));
    }
    return value;
}

// Derive the display transform in double precision. Calling PageToDevice on
// arbitrary PDF coordinates would round to integers and can overflow them.
class PageCoordinates
{
public:
    explicit PageCoordinates(FPDF_PAGE page)
    {
        double x0, y0, x1, y1, x2, y2;
        if (!FPDF_DeviceToPage(page, 0, 0, 1, 1, 0, 0, 0, &x0, &y0)
            || !FPDF_DeviceToPage(page, 0, 0, 1, 1, 0, 1, 0, &x1, &y1)
            || !FPDF_DeviceToPage(page, 0, 0, 1, 1, 0, 0, 1, &x2, &y2)) {
            return;
        }
        m_topLeft = QPointF(x0, y0);
        const QTransform toPage(x1 - x0, y1 - y0, x2 - x0, y2 - y0, x0, y0);
        m_toNormalized = toPage.inverted(&m_valid);
    }

    QPointF pageTopLeft() const { return m_topLeft; }

    std::optional<QPointF> point(double x, double y) const
    {
        if (!m_valid || !std::isfinite(x) || !std::isfinite(y)) {
            return std::nullopt;
        }
        const auto mapped = m_toNormalized.map(QPointF(x, y));
        if (!std::isfinite(mapped.x()) || !std::isfinite(mapped.y())) {
            return std::nullopt;
        }
        return QPointF(std::clamp(mapped.x(), 0.0, 1.0), std::clamp(mapped.y(), 0.0, 1.0));
    }

    std::optional<Okular::NormalizedRect> rect(const FS_RECTF &box) const
    {
        if (!(box.left < box.right && box.bottom < box.top)) {
            return std::nullopt;
        }
        const auto a = point(box.left, box.bottom), b = point(box.right, box.top);
        if (!a || !b) {
            return std::nullopt;
        }
        Okular::NormalizedRect area(std::min(a->x(), b->x()), std::min(a->y(), b->y()),
            std::max(a->x(), b->x()), std::max(a->y(), b->y()));
        return area.left < area.right && area.top < area.bottom ? std::optional(area) : std::nullopt;
    }

private:
    QTransform m_toNormalized;
    QPointF m_topLeft;
    bool m_valid = false;
};
}
