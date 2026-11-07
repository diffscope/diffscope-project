// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTEDITHINTS_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTEDITHINTS_H

#include <QFlags>
#include <QMetaObject>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    /** The presentation hints passed from editing commands to interaction strategies. */
    namespace DocumentEdit {
        Q_NAMESPACE_EXPORT(DSPXPROJECT_EXPORT)
        QML_ELEMENT

        /** The optional interaction presentation policies. */
        enum Hint {
            AtCursor = 0x01 /**< Positions the interaction at the cursor. */
        };
        /** The combined interaction presentation hints. */
        Q_DECLARE_FLAGS(Hints, Hint)
        Q_FLAG_NS(Hints)
        Q_DECLARE_OPERATORS_FOR_FLAGS(Hints)
    }

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTEDITHINTS_H
