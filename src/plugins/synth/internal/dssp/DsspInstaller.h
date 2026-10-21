// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPINSTALLER_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPINSTALLER_H

#include <QObject>
#include <QString>

class QWindow;

namespace Synth::Internal {

    class DsspInstaller : public QObject {
        Q_OBJECT
    public:
        static QString download(const QString &versionsDirectory, QWindow *parent);
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPINSTALLER_H
