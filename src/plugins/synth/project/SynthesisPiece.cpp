// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisPiece.h"
#include "SynthesisPiece_p.h"

#include <dspxmodelORM/SingingClip.h>

namespace Synth {

    SynthesisPiece::SynthesisPiece(dspx::SingingClip *clip, QObject *parent)
        : QObject(parent), d_ptr(new SynthesisPiecePrivate(this)) {
        Q_D(SynthesisPiece);
        d->clip = clip;
    }

    SynthesisPiece::~SynthesisPiece() = default;

    QUuid SynthesisPiece::id() const {
        Q_D(const SynthesisPiece);
        return d->id;
    }
    dspx::SingingClip *SynthesisPiece::singingClip() const {
        Q_D(const SynthesisPiece);
        return d->clip;
    }
    double SynthesisPiece::position() const {
        Q_D(const SynthesisPiece);
        return d->position;
    }
    double SynthesisPiece::length() const {
        Q_D(const SynthesisPiece);
        return d->length;
    }
    SynthesisPiece::State SynthesisPiece::state() const {
        Q_D(const SynthesisPiece);
        return d->state;
    }
    SynthesisTaskType SynthesisPiece::currentTaskType() const {
        Q_D(const SynthesisPiece);
        return d->currentTaskType;
    }
    QString SynthesisPiece::audioFilePath() const {
        Q_D(const SynthesisPiece);
        return d->audioFilePath;
    }
    QString SynthesisPiece::errorMessage() const {
        Q_D(const SynthesisPiece);
        return d->errorMessage;
    }
    QString SynthesisPiece::diagnosticFilePath() const {
        Q_D(const SynthesisPiece);
        return d->diagnosticFilePath;
    }

}

#include "moc_SynthesisPiece.cpp"
