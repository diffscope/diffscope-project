// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "PickTrackColorScenario.h"
#include "PickTrackColorScenario_p.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcPickTrackColorScenario, "diffscope.core.picktrackcolorscenario")

    PickTrackColorScenario::PickTrackColorScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new PickTrackColorScenarioPrivate) {
        Q_D(PickTrackColorScenario);
        d->q_ptr = this;
    }

    PickTrackColorScenario::~PickTrackColorScenario() = default;

    void PickTrackColorScenario::pickTrackColor(dspx::Track *track) const {
        Q_D(const PickTrackColorScenario);
        if (!track || !document() || !window())
            return;

        auto *targetDocument = document();
        TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcPickTrackColorScenario) << "Could not acquire a document read lock.";
            DocumentEditScenarioPrivate::sendOperationConflictNotification(window());
            return;
        }

        auto currentColorId = track->colorId();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PickTrackColorDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"colorId", currentColorId}
        });
        
        if (!DocumentEditScenarioPrivate::execDialog(dialog) || !readLock || document() != targetDocument)
            return;

        auto newColorId = dialog->property("colorId").toInt();

        qCInfo(lcPickTrackColorScenario) << "Pick track color" << track << "from" << currentColorId << "to" << newColorId;

        readLock.release();
        if (document() != targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Picking track color"), [=] {
            track->setColorId(newColorId);
            return true;
        }, [this] {
            qCWarning(lcPickTrackColorScenario) << "Failed to pick track color because the transaction could not start.";
            DocumentEditScenarioPrivate::sendOperationConflictNotification(window());
        });
    }

}

#include "moc_PickTrackColorScenario.cpp"