// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_H
#define DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_H

#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <synth/SynthesisModel.h>
#include <synth/synthglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class SingingClip;
}

namespace Synth {

    namespace Internal {
        class SynthesisProjectAddOn;
    }

    class SynthesisPiece;
    class ProjectSynthesisContextPrivate;

    class SYNTH_EXPORT ProjectSynthesisContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectSynthesisContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(QList<Synth::SynthesisPiece *> pieces READ pieces NOTIFY piecesChanged)
        Q_PROPERTY(int synthesizingPieceCount READ synthesizingPieceCount NOTIFY pieceCountsChanged)
        Q_PROPERTY(int queuedPieceCount READ queuedPieceCount NOTIFY pieceCountsChanged)

    public:
        ~ProjectSynthesisContext() override;

        static ProjectSynthesisContext *of(Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;
        /**
         * Returns all document pieces known to the synthesis scheduler.
         *
         * This includes pieces outside their clips' current clipStart/clipLength
         * ranges. Such pieces remain available for inspection but are not sent
         * to synthesis tasks until they intersect the current clip range.
         */
        QList<SynthesisPiece *> pieces() const;
        /** Returns every ordered piece of a clip, including pieces outside its current clip range. */
        QList<SynthesisPiece *> piecesForClip(dspx::SingingClip *clip) const;
        /** Returns pieces intersecting a clip-content-relative range. */
        QList<SynthesisPiece *> piecesInRange(dspx::SingingClip *clip, double position, double length) const;
        int synthesizingPieceCount() const;
        int queuedPieceCount() const;

        Q_INVOKABLE void resynthesizeProject(Synth::SynthesisTaskType fromType, bool readCache = true, bool writeCache = true);
        Q_INVOKABLE void resynthesizeClip(dspx::SingingClip *clip, Synth::SynthesisTaskType fromType, bool readCache = true, bool writeCache = true);
        Q_INVOKABLE void resynthesizePiece(Synth::SynthesisPiece *piece, Synth::SynthesisTaskType fromType, bool readCache = true, bool writeCache = true);
        Q_INVOKABLE bool cancelPiece(Synth::SynthesisPiece *piece);
        Q_INVOKABLE void cancelAll();

    Q_SIGNALS:
        void piecesChanged();
        void pieceChanged(Synth::SynthesisPiece *piece);
        void pieceCountsChanged();

    private:
        explicit ProjectSynthesisContext(Core::ProjectWindowInterface *windowHandle, QObject *parent = nullptr);

        QScopedPointer<ProjectSynthesisContextPrivate> d_ptr;

        friend class Internal::SynthesisProjectAddOn;
    };

}

#endif // DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_H
