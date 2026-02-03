#ifndef DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H
#define DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H

#include <QColor>
#include <QList>

#include <coreplugin/TrackColorSchema.h>

namespace Core {

    class TrackColorSchemaPrivate {
        Q_DECLARE_PUBLIC(TrackColorSchema)
    public:
        TrackColorSchema *q_ptr;
        QList<QColor> colors;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H
