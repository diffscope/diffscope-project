// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_SHORTNOTEPROBLEMADDON_H
#define DIFFSCOPE_COREPLUGIN_SHORTNOTEPROBLEMADDON_H

#include <set>
#include <utility>

#include <QHash>
#include <QPointer>
#include <QSet>

#include <CoreApi/windowinterface.h>

#include <dspxmodelORM/Handle.h>

namespace DspxProject {
    class Document;
    struct DocumentProblem;
}

namespace dspx {
    class Note;
    class SingingClip;
    class Track;
}

namespace Core::Internal {

    class ShortNoteProblemAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ShortNoteProblemAddOn(QObject *parent = nullptr);
        ~ShortNoteProblemAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

    private:
        struct ClipState {
            QPointer<dspx::SingingClip> clip;
            QSet<dspx::Handle> notes;
        };

        struct NoteState {
            QPointer<dspx::Note> note;
            dspx::Handle clip{};
            int length = -1;
            qint64 gap = 0;
        };

        using LengthIndex = std::set<std::pair<qint64, dspx::Handle>>;

        void watchTrack(dspx::Track *track);
        void unwatchTrack(dspx::Track *track);
        void markNoteDirty(dspx::Note *note);
        void scheduleRefresh();
        void refreshProblems();
        void refreshClip(dspx::Handle handle);
        void refreshNote(dspx::Handle handle);
        bool fillGap(const DspxProject::DocumentProblem &problem, bool extendPrevious);

        QPointer<DspxProject::Document> m_document;
        QHash<dspx::Handle, QSet<dspx::Handle>> m_trackClips;
        QHash<dspx::Handle, ClipState> m_clips;
        QHash<dspx::Handle, NoteState> m_notes;
        QSet<dspx::Handle> m_dirtyClips;
        QSet<dspx::Handle> m_dirtyNotes;
        LengthIndex m_noteLengths;
        LengthIndex m_gapLengths;
        int m_noteThreshold = 0;
        int m_gapThreshold = 0;
        bool m_refreshQueued = false;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SHORTNOTEPROBLEMADDON_H
