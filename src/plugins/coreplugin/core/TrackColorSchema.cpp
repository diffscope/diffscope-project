#include "TrackColorSchema.h"
#include "TrackColorSchema_p.h"

namespace Core {
    TrackColorSchema::TrackColorSchema(QObject *parent) : QObject(parent), d_ptr(new TrackColorSchemaPrivate) {
        Q_D(TrackColorSchema);
        d->q_ptr = this;
        d->colors = {
            QColor::fromRgb(0x5566ff),
            QColor::fromRgb(0x8f50ef),
            QColor::fromRgb(0xb641c3),
            QColor::fromRgb(0xc64291),
            QColor::fromRgb(0xcf4553),
            QColor::fromRgb(0xae693a),
            QColor::fromRgb(0x887f2d),
            QColor::fromRgb(0x668a2e),
            QColor::fromRgb(0x3b9331),
            QColor::fromRgb(0x319257),
            QColor::fromRgb(0x2f8d84),
            QColor::fromRgb(0x3c84b4),
        };
    }

    TrackColorSchema::~TrackColorSchema() = default;

    QList<QColor> TrackColorSchema::colors() const {
        Q_D(const TrackColorSchema);
        return d->colors;
    }

    void TrackColorSchema::setColors(const QList<QColor> &colors) {
        Q_D(TrackColorSchema);
        if (d->colors == colors)
            return;
        d->colors = colors;
        emit colorsChanged(d->colors);
    }

}

#include "moc_TrackColorSchema.cpp"
