// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class ProjectTimeline;

    class Document;
    class EditKeySignatureStrategy;
    class EditKeySignatureScenarioPrivate;

    /**
     * Coordinates key signature editing in transactions.
     * Window arguments are optional dialog parents. Positions use project ticks.
     * @pre Timeline arguments are non-null and refer to this document.
     */
    class DSPXPROJECT_EXPORT EditKeySignatureScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditKeySignatureScenario)


    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document and strategy are non-null and valid during use.
         */
        EditKeySignatureScenario(Document *document, EditKeySignatureStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~EditKeySignatureScenario() override;

        /** Edits the key signature effective at the timeline playhead. */
        Q_INVOKABLE void editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        /** Requests key signature values and inserts or modifies data according to doInsertNew. */
        Q_INVOKABLE void editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int tonality, int mode, int accidentalType, DocumentEdit::Hints hints = {}) const;
        /** Requests insertion at position using the effective key signature as initial values. */
        Q_INVOKABLE void insertKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        /** Requests modification of the effective key signature, inserting if none exists. */
        Q_INVOKABLE void modifyExistingKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<EditKeySignatureScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_H
