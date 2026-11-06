// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace dspx {
    class Track;
}

namespace DspxProject {

    class Document;
    class PickTrackColorStrategy;
    class PickTrackColorScenarioPrivate;

    class DSPXPROJECT_EXPORT PickTrackColorScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PickTrackColorScenario)

    public:
        PickTrackColorScenario(Document *document, PickTrackColorStrategy *strategy, QObject *parent = nullptr);
        ~PickTrackColorScenario() override;

        Q_INVOKABLE void pickTrackColor(QQuickWindow *window, dspx::Track *track, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<PickTrackColorScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_H
