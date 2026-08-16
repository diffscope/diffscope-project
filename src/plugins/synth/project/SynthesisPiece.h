// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPIECE_H
#define DIFFSCOPE_SYNTH_SYNTHESISPIECE_H

#include <QObject>
#include <QScopedPointer>
#include <QUuid>

#include <synth/SynthesisModel.h>
#include <synth/synthglobal.h>

namespace dspx {
    class SingingClip;
}

namespace Synth {

    namespace Internal {
        class SynthesisProjectAddOn;
    }

    class SynthesisPiecePrivate;

    class SYNTH_EXPORT SynthesisPiece : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SynthesisPiece)
        Q_PROPERTY(QUuid id READ id CONSTANT)
        Q_PROPERTY(dspx::SingingClip *singingClip READ singingClip CONSTANT)
        Q_PROPERTY(double position READ position NOTIFY rangeChanged)
        Q_PROPERTY(double length READ length NOTIFY rangeChanged)
        Q_PROPERTY(State state READ state NOTIFY stateChanged)
        Q_PROPERTY(Synth::SynthesisTaskType currentTaskType READ currentTaskType NOTIFY stateChanged)
        Q_PROPERTY(QString audioFilePath READ audioFilePath NOTIFY audioFileChanged)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)

    public:
        enum State {
            Idle,
            Stale,
            Queued,
            Synthesizing,
            Ready,
            Failed,
        };
        Q_ENUM(State)

        ~SynthesisPiece() override;

        QUuid id() const;
        dspx::SingingClip *singingClip() const;
        double position() const;
        double length() const;
        State state() const;
        SynthesisTaskType currentTaskType() const;
        QString audioFilePath() const;
        QString errorMessage() const;

    Q_SIGNALS:
        void rangeChanged();
        void stateChanged();
        void audioFileChanged();

    private:
        explicit SynthesisPiece(dspx::SingingClip *clip, QObject *parent = nullptr);

        QScopedPointer<SynthesisPiecePrivate> d_ptr;

        friend class Internal::SynthesisProjectAddOn;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPIECE_H
