// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditItemsScenario.h"
#include "EditItemsScenario_p.h"

#include <QPointer>
#include <QQuickWindow>

#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditItemsStrategy.h>
#include <dspxproject/ParameterResamplingScenario.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    EditItemsScenario::EditItemsScenario(Document *document, EditItemsStrategy *strategy, ParameterResamplingScenario *resampling, QObject *parent)
        : QObject(parent), d_ptr(new EditItemsScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy, .resampling = resampling}) {
    }

    EditItemsScenario::~EditItemsScenario() = default;

    QString EditItemsScenario::splitTransactionName(const Document *document) {
        switch (document->selectionModel()->selectionType()) {
            case dspx::SelectionModel::ST_Clip:
                return tr("Splitting clip");
            case dspx::SelectionModel::ST_Note:
                return tr("Splitting note");
            default:
                return {};
        }
    }

    void EditItemsScenario::splitItems(int position) const {
        Q_D(const EditItemsScenario);
        const QPointer<Document> document(d->document);
        const auto name = splitTransactionName(document);
        if (name.isEmpty()) {
            return;
        }
        QList<QObject *> inserted;
        document->transactionController()->beginScopedTransaction(name, [&] {
            inserted = document->splitItems(position);
            return !inserted.isEmpty();
        }, [strategy = d->strategy] {
            strategy->reportOperationConflict();
        });
        if (document && !inserted.isEmpty()) {
            document->selectItems(inserted);
        }
    }

    void EditItemsScenario::bounceToClip(QQuickWindow *window) const {
        Q_D(const EditItemsScenario);
        const QPointer<Document> document(d->document);
        const QPointer<const EditItemsScenario> scenario(this);
        const QPointer<QQuickWindow> dialogWindow(window);
        auto selection = document->selectionModel();
        if (selection->selectionType() != dspx::SelectionModel::ST_Clip || !selection->clipSelectionModel()->selectedSingingClipCount()) {
            return;
        }
        auto clipSelection = selection->clipSelectionModel();
        const auto clips = clipSelection->selectedItems();
        const auto targetValid = [&] {
            return scenario && document && (!window || dialogWindow) &&
                   selection->selectionType() == dspx::SelectionModel::ST_Clip && clipSelection->selectedItems() == clips;
        };
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            d->strategy->reportOperationConflict();
            return;
        }
        const auto risks = document->bounceRisks();
        if (!risks || !targetValid()) {
            return;
        }
        using BounceRisk = Document::BounceRisk;
        if (risks->testFlag(BounceRisk::SingerLayoutMismatch) && (!d->strategy->confirmSingerLayoutChange(dialogWindow) || !readLock || !targetValid())) {
            return;
        }
        if (risks->testFlag(BounceRisk::DataLoss) && (!d->strategy->confirmBounceDataLoss(dialogWindow) || !readLock || !targetValid())) {
            return;
        }
        if (!d->resampling->confirmBounce(dialogWindow, risks->testFlag(BounceRisk::ParameterResampling)) || !readLock || !targetValid()) {
            return;
        }
        readLock.release();
        if (!targetValid()) {
            return;
        }
        document->transactionController()->beginScopedTransaction(tr("Bouncing to clip"), [&] {
            if (!targetValid()) {
                return false;
            }
            const auto current = document->bounceRisks();
            if (!current || (*current & ~*risks)) {
                return false;
            }
            return document->bounceToClip();
        }, [strategy = d->strategy] {
            strategy->reportOperationConflict();
        });
    }

}

#include "moc_EditItemsScenario.cpp"
