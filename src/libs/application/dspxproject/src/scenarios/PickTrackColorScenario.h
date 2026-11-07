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

    /** Coordinates track color selection and commits changes in a transaction. */
    class DSPXPROJECT_EXPORT PickTrackColorScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PickTrackColorScenario)

    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document and strategy are non-null and valid during use.
         */
        PickTrackColorScenario(Document *document, PickTrackColorStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~PickTrackColorScenario() override;

        /**
         * Requests a color with an optional dialog window and applies accepted changes.
         * Performs no operation for a track in another model.
         * @pre track is non-null.
         */
        Q_INVOKABLE void pickTrackColor(QQuickWindow *window, dspx::Track *track, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<PickTrackColorScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PICKTRACKCOLORSCENARIO_H
