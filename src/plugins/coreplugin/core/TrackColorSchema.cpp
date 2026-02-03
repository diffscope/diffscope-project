#include "TrackColorSchema.h"
#include "TrackColorSchema_p.h"

namespace Core {
    TrackColorSchema::TrackColorSchema(QObject *parent) : QObject(parent), d_ptr(new TrackColorSchemaPrivate) {
        Q_D(TrackColorSchema);
        d->q_ptr = this;
        d->colors = {
            QColor::fromRgb(0x316f50),
            QColor::fromRgb(0x306e6e),
            QColor::fromRgb(0x466585),
            QColor::fromRgb(0x5d5d9e),
            QColor::fromRgb(0x765696),
            QColor::fromRgb(0x8c4c8c),
            QColor::fromRgb(0x8f4d6e),
            QColor::fromRgb(0x914e4e),
            QColor::fromRgb(0x7a5c3d),
            QColor::fromRgb(0x66662a),
            QColor::fromRgb(0x4d6c2e),
            QColor::fromRgb(0x316f31),
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
