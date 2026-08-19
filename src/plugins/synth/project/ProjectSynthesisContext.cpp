// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectSynthesisContext.h"

#include <coreplugin/ProjectWindowInterface.h>

#include <synth/internal/SynthesisProjectAddOn.h>
#include <synth/private/ProjectSynthesisContext_p.h>

namespace Synth {

    ProjectSynthesisContext::ProjectSynthesisContext(Core::ProjectWindowInterface *windowHandle, QObject *parent)
        : QObject(parent), d_ptr(new ProjectSynthesisContextPrivate(this)) {
        Q_D(ProjectSynthesisContext);
        d->windowHandle = windowHandle;
    }

    ProjectSynthesisContext::~ProjectSynthesisContext() = default;

    ProjectSynthesisContext *ProjectSynthesisContext::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle ? windowHandle->getFirstObject<ProjectSynthesisContext>() : nullptr;
    }

    Core::ProjectWindowInterface *ProjectSynthesisContext::windowHandle() const {
        Q_D(const ProjectSynthesisContext);
        return d->windowHandle;
    }

    QList<SynthesisPiece *> ProjectSynthesisContext::pieces() const {
        Q_D(const ProjectSynthesisContext);
        return d->controller ? d->controller->pieces() : QList<SynthesisPiece *>{};
    }

    QList<SynthesisPiece *> ProjectSynthesisContext::piecesForClip(dspx::SingingClip *clip) const {
        Q_D(const ProjectSynthesisContext);
        return d->controller ? d->controller->piecesForClip(clip) : QList<SynthesisPiece *>{};
    }

    QList<SynthesisPiece *> ProjectSynthesisContext::piecesInRange(dspx::SingingClip *clip, double position, double length) const {
        Q_D(const ProjectSynthesisContext);
        return d->controller ? d->controller->piecesInRange(clip, position, length)
                             : QList<SynthesisPiece *>{};
    }

    int ProjectSynthesisContext::synthesizingPieceCount() const {
        Q_D(const ProjectSynthesisContext);
        return d->controller ? d->controller->synthesizingPieceCount() : 0;
    }

    int ProjectSynthesisContext::queuedPieceCount() const {
        Q_D(const ProjectSynthesisContext);
        return d->controller ? d->controller->queuedPieceCount() : 0;
    }

    void ProjectSynthesisContext::resynthesizeProject(SynthesisTaskType fromType, bool readCache, bool writeCache) {
        Q_D(ProjectSynthesisContext);
        if (d->controller)
            d->controller->resynthesizeProject(fromType, {readCache, writeCache, 0});
    }

    void ProjectSynthesisContext::resynthesizeClip(dspx::SingingClip *clip, SynthesisTaskType fromType, bool readCache, bool writeCache) {
        Q_D(ProjectSynthesisContext);
        if (d->controller)
            d->controller->resynthesizeClip(clip, fromType, {readCache, writeCache, 0});
    }

    void ProjectSynthesisContext::resynthesizePiece(SynthesisPiece *piece, SynthesisTaskType fromType, bool readCache, bool writeCache) {
        Q_D(ProjectSynthesisContext);
        if (d->controller)
            d->controller->resynthesizePiece(piece, fromType, {readCache, writeCache, 0});
    }

    bool ProjectSynthesisContext::cancelPiece(SynthesisPiece *piece) {
        Q_D(ProjectSynthesisContext);
        return d->controller && d->controller->cancelPiece(piece);
    }

    bool ProjectSynthesisContext::cancelPieceTask(SynthesisPiece *piece) {
        Q_D(ProjectSynthesisContext);
        return d->controller && d->controller->cancelPieceTask(piece);
    }

    void ProjectSynthesisContext::cancelAll() {
        Q_D(ProjectSynthesisContext);
        if (d->controller)
            d->controller->cancelAll();
    }

}

#include "moc_ProjectSynthesisContext.cpp"
