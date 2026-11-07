// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class ProjectTimeline;

    class Document;
    class EditLoopStrategy;
    class EditLoopScenarioPrivate;

    /** Coordinates loop range editing in a transaction. */
    class DSPXPROJECT_EXPORT EditLoopScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditLoopScenario)


    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document and strategy are non-null and valid during use.
         */
        EditLoopScenario(Document *document, EditLoopStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~EditLoopScenario() override;

        /**
         * Requests loop values with an optional dialog window and commits accepted changes.
         * @pre timeline is non-null and refers to this document.
         */
        Q_INVOKABLE void editLoop(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<EditLoopScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITLOOPSCENARIO_H
