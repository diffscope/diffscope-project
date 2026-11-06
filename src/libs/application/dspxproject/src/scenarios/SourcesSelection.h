// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H
#define DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H

#include <QList>
#include <QMetaObject>
#include <QString>
#include <qqmlintegration.h>

#include <opendspx/singer.h>

#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    struct DSPXPROJECT_EXPORT SourcesSelection {
        Q_GADGET
        QML_VALUE_TYPE(sourcesSelection)
    public:
        QString architectureId;
        QList<opendspx::SingerRef> singers;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H
