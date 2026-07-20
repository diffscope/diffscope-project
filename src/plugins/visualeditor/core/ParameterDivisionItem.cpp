#include "ParameterDivisionItem.h"
#include "ParameterDivisionItem_p.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QPainter>

namespace VisualEditor {

    ParameterDivisionItem::ParameterDivisionItem(QQuickItem *parent)
        : QQuickPaintedItem(parent), d_ptr(new ParameterDivisionItemPrivate) {
        Q_D(ParameterDivisionItem);
        d->q_ptr = this;
        setAntialiasing(false);
    }

    ParameterDivisionItem::~ParameterDivisionItem() = default;

    Core::ParameterInfo ParameterDivisionItem::parameterInfo() const {
        Q_D(const ParameterDivisionItem);
        return d->parameterInfo;
    }

    void ParameterDivisionItem::setParameterInfo(const Core::ParameterInfo &parameterInfo) {
        Q_D(ParameterDivisionItem);
        if (d->parameterInfo == parameterInfo)
            return;
        d->parameterInfo = parameterInfo;
        update();
        Q_EMIT parameterInfoChanged();
    }

    QColor ParameterDivisionItem::color() const {
        Q_D(const ParameterDivisionItem);
        return d->color;
    }

    void ParameterDivisionItem::setColor(const QColor &color) {
        Q_D(ParameterDivisionItem);
        if (d->color == color)
            return;
        d->color = color;
        update();
        Q_EMIT colorChanged();
    }

    qreal ParameterDivisionItem::lineLength() const {
        Q_D(const ParameterDivisionItem);
        return d->lineLength;
    }

    void ParameterDivisionItem::setLineLength(qreal lineLength) {
        Q_D(ParameterDivisionItem);
        lineLength = std::max<qreal>(0.0, lineLength);
        if (qFuzzyCompare(d->lineLength, lineLength))
            return;
        d->lineLength = lineLength;
        update();
        Q_EMIT lineLengthChanged();
    }

    void ParameterDivisionItem::paint(QPainter *painter) {
        Q_D(ParameterDivisionItem);
        const auto &info = d->parameterInfo;
        if (!info.showDivision || info.divisionValue <= 0 || height() <= 0 || width() <= 0 || !d->color.isValid())
            return;
        const int bottom = std::min(info.bottomValue, info.topValue);
        const int top = std::max(info.bottomValue, info.topValue);
        const qint64 first = qint64(std::ceil(double(bottom) / info.divisionValue)) * info.divisionValue;
        const qint64 count = first > top ? 0 : (qint64(top) - first) / info.divisionValue + 1;
        if (count <= 0)
            return;
        const qint64 stride = std::max<qint64>(1, count / std::max<qint64>(1, qint64(height()) * 2));
        const qreal left = std::max<qreal>(0.0, width() - d->lineLength);
        int previousPixel = std::numeric_limits<int>::min();
        painter->setPen(QPen(d->color, 1.0));
        for (qint64 i = 0; i < count; i += stride) {
            const qint64 raw = first + i * info.divisionValue;
            const double normalized = info.invokeNormalize(int(raw));
            if (!std::isfinite(normalized))
                continue;
            const qreal y = (1.0 - normalized) * height();
            const int pixel = qRound(y);
            if (pixel == previousPixel)
                continue;
            previousPixel = pixel;
            painter->drawLine(QPointF(left, y), QPointF(width(), y));
        }
    }
}

#include "moc_ParameterDivisionItem.cpp"
