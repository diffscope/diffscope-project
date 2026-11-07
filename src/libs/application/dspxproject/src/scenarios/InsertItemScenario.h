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

    /**
     * Coordinates item insertion and selection with edit transactions.
     * Window arguments are optional dialog parents. Cancelled interactions perform no edit.
     * @pre Timeline arguments are non-null and refer to this document.
     */
    class DSPXPROJECT_EXPORT InsertItemScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(InsertItemScenario)
    public:
        /** The required callbacks for track colors and default lyrics. */
        struct Options {
            /** Returns the track color count. A nonpositive count skips color assignment. */
            std::function<int()> trackColorCount;
            /** Returns the default lyric for a non-null singing clip. */
            std::function<QString(dspx::SingingClip *clip)> defaultLyricForClip;
        };

        /**
         * Creates a scenario with borrowed dependencies and copied callbacks.
         * @pre document and strategy are non-null and valid during use.
         * @pre Both options callbacks are callable and their captures remain valid during use.
         */
        InsertItemScenario(Document *document, InsertItemStrategy *strategy, const Options &options, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~InsertItemScenario() override;

        /** Adds a default track after the current track, or at the end if no track is current. */
        Q_INVOKABLE void addTrack() const;
        /** Requests a track count, index, and name, then inserts and selects the tracks. */
        Q_INVOKABLE void insertTrack(QQuickWindow *window, DocumentEdit::Hints hints = {}) const;
        /** Requests a label initialized at the timeline playhead, then inserts and selects it. */
        Q_INVOKABLE void insertLabel(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        /** Requests a singing clip and destination track, then inserts and selects the clip. */
        Q_INVOKABLE void insertSingingClip(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints = {}) const;
        /**
         * Requests a note and inserts it into clip, then selects the note.
         * Uses the active note sequence if clip is nullptr. Performs no operation if
         * no sequence is available or a supplied clip belongs to another model.
         */
        Q_INVOKABLE void insertNote(QQuickWindow *window, ProjectTimeline *timeline, dspx::SingingClip *clip = nullptr, DocumentEdit::Hints hints = {}) const;
        /**
         * Requests an edited phoneme and inserts it into note.
         * Uses the first selected note if note is nullptr. Performs no operation if
         * no note is available or a supplied note belongs to another model.
         */
        Q_INVOKABLE void insertPhoneme(QQuickWindow *window, dspx::Note *note = nullptr, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<InsertItemScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_INSERTITEMSCENARIO_H
