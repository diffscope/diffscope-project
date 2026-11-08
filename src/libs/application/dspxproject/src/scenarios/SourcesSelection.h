// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H
#define DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H

#include <optional>

#include <QList>
#include <QMetaObject>
#include <QString>
#include <qqmlintegration.h>

#include <opendspx/sources.h>

#include <dspxproject/dspxprojectglobal.h>

namespace dspx {
    class SingingClip;
}

namespace DspxProject {

    /** The architecture and ordered singers to apply to singing clips. */
    struct DSPXPROJECT_EXPORT SourcesSelection {
        Q_GADGET
        QML_VALUE_TYPE(sourcesSelection)
        /** The architecture identifier, empty to remove sources. */
    public:
        QString architectureId;
        /** The singer definitions, empty to remove sources. All nested references must be non-null. */
        QList<opendspx::SingerRef> singers;

        /** Returns source data, or nullopt if the architecture or singer list is empty. */
        std::optional<opendspx::Sources> toOpenDSPX() const;
        /** Applies sources within the caller's active transaction. Returns whether a non-null clip's sources changed. */
        bool applyTo(dspx::SingingClip *clip) const;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_SOURCESSELECTION_H
