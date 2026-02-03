#include "TrackColorSchema.h"
#include "TrackColorSchema_p.h"

namespace Core {
    TrackColorSchema::TrackColorSchema(QObject *parent) : QObject(parent), d_ptr(new TrackColorSchemaPrivate) {
        Q_D(TrackColorSchema);
        d->q_ptr = this;
        d->colors = {
            QColor::fromRgb(0x555eab),
            QColor::fromRgb(0x7554a7),
            QColor::fromRgb(0x8e4b95),
            QColor::fromRgb(0x944a76),
            QColor::fromRgb(0x964b52),
            QColor::fromRgb(0x825b41),
            QColor::fromRgb(0x696435),
            QColor::fromRgb(0x546934),
            QColor::fromRgb(0x3d6f38),
            QColor::fromRgb(0x376d4c),
            QColor::fromRgb(0x376e68),
            QColor::fromRgb(0x426983),
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
