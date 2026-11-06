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

    class DSPXPROJECT_EXPORT EditKeySignatureScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(EditKeySignatureScenario)


    public:
        EditKeySignatureScenario(Document *document, EditKeySignatureStrategy *strategy, QObject *parent = nullptr);
        ~EditKeySignatureScenario() override;

        Q_INVOKABLE void editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int tonality, int mode, int accidentalType, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void modifyExistingKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<EditKeySignatureScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESCENARIO_H
