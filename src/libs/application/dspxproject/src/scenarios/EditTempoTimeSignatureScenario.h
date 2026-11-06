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

    class DSPXPROJECT_EXPORT EditTempoTimeSignatureScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditTempoTimeSignatureScenario)


    public:
        EditTempoTimeSignatureScenario(Document *document, EditTempoTimeSignatureStrategy *strategy, QObject *parent = nullptr);
        ~EditTempoTimeSignatureScenario() override;

        Q_INVOKABLE void editTempo(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void editTempo(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, double initialTempo, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void modifyExistingTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

        Q_INVOKABLE void editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int initialNumerator, int initialDenominator, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void modifyExistingTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<EditTempoTimeSignatureScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITTEMPOTIMESIGNATURESCENARIO_H
