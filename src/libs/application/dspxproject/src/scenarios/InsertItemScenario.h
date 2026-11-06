// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_H

#include <functional>

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace dspx {
    class Note;
    class SingingClip;
}

namespace DspxProject {

    class Document;
    class InsertItemScenarioPrivate;
    class InsertItemStrategy;
    class ProjectTimeline;

    class DSPXPROJECT_EXPORT InsertItemScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(InsertItemScenario)
    public:
        struct Options {
            std::function<int()> trackColorCount;
            std::function<QString(dspx::SingingClip *clip)> defaultLyricForClip;
        };

        InsertItemScenario(Document *document, InsertItemStrategy *strategy, const Options &options, QObject *parent = nullptr);
        ~InsertItemScenario() override;

        Q_INVOKABLE void addTrack() const;
        Q_INVOKABLE void insertTrack(QQuickWindow *window, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertLabel(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertSingingClip(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertNote(QQuickWindow *window, ProjectTimeline *timeline, dspx::SingingClip *clip = nullptr, DocumentEdit::Hints hints = {}) const;
        Q_INVOKABLE void insertPhoneme(QQuickWindow *window, dspx::Note *note = nullptr, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<InsertItemScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_H
