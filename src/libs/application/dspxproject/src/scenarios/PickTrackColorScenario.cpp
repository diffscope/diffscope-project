// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PickTrackColorScenario.h"
#include "PickTrackColorScenario_p.h"

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>

#include <dspxproject/Document.h>
#include <dspxproject/PickTrackColorStrategy.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcPickTrackColorScenario, "diffscope.dspxproject.picktrackcolorscenario")

    PickTrackColorScenario::PickTrackColorScenario(Document *document, PickTrackColorStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new PickTrackColorScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy}) {
    }

    PickTrackColorScenario::~PickTrackColorScenario() = default;

    void PickTrackColorScenario::pickTrackColor(QQuickWindow *window, dspx::Track *track, DocumentEdit::Hints hints) const {
        Q_D(const PickTrackColorScenario);
        const QPointer<Document> targetDocument(d->document);
        if (!track || track->model() != targetDocument->model()) {
            return;
        }
        const QPointer<dspx::Track> targetTrack(track);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcPickTrackColorScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        auto currentColorId = track->colorId();

        const auto color = d->strategy->requestColor(currentColorId, dialogWindow, hints);
        if (!color || !readLock || !targetDocument || !targetTrack || *color == currentColorId)
            return;

        const auto newColorId = *color;

        qCInfo(lcPickTrackColorScenario) << "Pick track color" << track << "from" << currentColorId << "to" << newColorId;

        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Picking track color"), [=] {
            if (!targetTrack || targetTrack->colorId() == newColorId) {
                return false;
            }
            targetTrack->setColorId(newColorId);
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcPickTrackColorScenario) << "Failed to pick track color because the transaction could not start.";
            strategy->reportOperationConflict();
        });
    }

}

#include "moc_PickTrackColorScenario.cpp"
