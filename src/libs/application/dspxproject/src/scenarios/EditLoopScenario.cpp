// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditLoopScenario.h"
#include "EditLoopScenario_p.h"

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/Model.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditLoopStrategy.h>
#include <dspxproject/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcEditLoopScenario, "diffscope.dspxproject.editloopscenario")

    EditLoopScenario::EditLoopScenario(Document *document, EditLoopStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new EditLoopScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy}) {
    }

    EditLoopScenario::~EditLoopScenario() = default;

    void EditLoopScenario::editLoop(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        Q_D(const EditLoopScenario);

        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcEditLoopScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        auto model = d->document->model();
        auto startPosition = model->loopStart();
        auto endPosition = startPosition + model->loopLength();
        auto loopEnabled = model->loopEnabled();

        const auto result = d->strategy->requestLoop({loopEnabled, startPosition, endPosition}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument)
            return;

        loopEnabled = result->enabled;
        startPosition = result->start;
        endPosition = result->end;

        if (startPosition < 0) {
            qCWarning(lcEditLoopScenario) << "Invalid loop start" << startPosition;
            return;
        }

        if (endPosition <= startPosition) {
            qCWarning(lcEditLoopScenario) << "Invalid loop end" << endPosition << "for start" << startPosition;
            return;
        }

        const auto loopLength = endPosition - startPosition;
        if (model->loopEnabled() == loopEnabled && model->loopStart() == startPosition && model->loopLength() == loopLength)
            return;

        qCInfo(lcEditLoopScenario) << "Edit loop" << loopEnabled << startPosition << loopLength;

        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Editing loop"), [=] {
            auto dspxModel = d->document->model();
            dspxModel->setLoopEnabled(loopEnabled);
            dspxModel->setLoopStart(startPosition);
            dspxModel->setLoopLength(loopLength);
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcEditLoopScenario) << "Failed to edit loop because the transaction could not start.";
            strategy->reportOperationConflict();
        });
    }

}

#include "moc_EditLoopScenario.cpp"
