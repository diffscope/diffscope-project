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

    /** The architecture and ordered singers to apply to singing clips. */
    struct DSPXPROJECT_EXPORT SourcesSelection {
        Q_GADGET
        QML_VALUE_TYPE(sourcesSelection)
    public:
        /** The architecture identifier, empty to remove sources. */
        QString architectureId;
        /** The singer definitions, empty to remove sources. All nested references must be non-null. */
        QList<opendspx::SingerRef> singers;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H
