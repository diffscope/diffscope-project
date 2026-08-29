// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H
#define DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H

#include <coreplugin/TrackColorSchema.h>

#include <QColor>
#include <QList>

namespace Core {

    class TrackColorSchemaPrivate {
        Q_DECLARE_PUBLIC(TrackColorSchema)
    public:
        TrackColorSchema *q_ptr;
        QList<QColor> colors;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_TRACKCOLORSCHEMA_P_H
