// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class ProjectTimeline;

    class Document;
    class EditTempoTimeSignatureStrategy;
    class EditTempoTimeSignatureScenarioPrivate;

    /**
     * Coordinates tempo and meter editing in transactions.
     * Window arguments are optional dialog parents. Positions use project ticks.
     * Meter insertions use the measure containing the accepted tick position.
     * @pre Timeline arguments are non-null and refer to this document.
     */
    class DSPXPROJECT_EXPORT EditTempoTimeSignatureScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditTempoTimeSignatureScenario)


    public:
        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document and strategy are non-null and valid during use.
         */
        EditTempoTimeSignatureScenario(Document *document, EditTempoTimeSignatureStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~EditTempoTimeSignatureScenario() override;

        /** Edits the tempo effective at the timeline playhead. */
        Q_INVOKABLE void editTempo(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        /** Requests tempo values and inserts or modifies data according to doInsertNew. */
        Q_INVOKABLE void editTempo(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, double initialTempo, DocumentEdit::Hints hints = {}) const;
        /** Requests tempo insertion at position using the effective tempo as the initial value. */
        Q_INVOKABLE void insertTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        /** Requests modification of the tempo effective at position. */
        Q_INVOKABLE void modifyExistingTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

        /** Edits the time signature effective at the timeline playhead. */
        Q_INVOKABLE void editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        /** Requests meter values and inserts or modifies data according to doInsertNew. */
        Q_INVOKABLE void editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int initialNumerator, int initialDenominator, DocumentEdit::Hints hints = {}) const;
        /** Requests meter insertion at position using the effective meter as initial values. */
        Q_INVOKABLE void insertTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        /** Requests modification of the time signature effective at position. */
        Q_INVOKABLE void modifyExistingTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<EditTempoTimeSignatureScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_H
