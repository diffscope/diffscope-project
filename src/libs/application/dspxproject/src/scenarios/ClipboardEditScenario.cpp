// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClipboardEditScenario.h"
#include "ClipboardEditScenario_p.h"

#include <QPointer>
#include <QQuickWindow>

#include <dspxmodelSelectionModel/SelectionModel.h>

#include <dspxproject/Clipboard.h>
#include <dspxproject/ClipboardEditStrategy.h>
#include <dspxproject/ParameterResamplingScenario.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    ClipboardEditScenario::ClipboardEditScenario(Document *document, ClipboardEditStrategy *strategy, Clipboard *clipboard, ParameterResamplingScenario *resampling, QObject *parent)
        : QObject(parent), d_ptr(new ClipboardEditScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy, .clipboard = clipboard, .resampling = resampling}) {
        Q_D(ClipboardEditScenario);
        const auto update = [d] { d->updatePasteAvailable(); };
        connect(clipboard, &Clipboard::changed, this, update);
        connect(document, &Document::pasteTargetsChanged, this, update);
        connect(document->transactionController(), &Core::TransactionController::accessStateChanged, this, update);
        d->updatePasteAvailable();
    }

    ClipboardEditScenario::~ClipboardEditScenario() = default;

    void ClipboardEditScenarioPrivate::updatePasteAvailable() {
        const auto target = document->resolvePasteTarget(clipboard->availablePasteTypes());
        const bool available = target && (target->type() != ClipboardData::FreeParameter ||
                                          document->transactionController()->canAcquireReadLock());
        if (pasteAvailable != available) {
            pasteAvailable = available;
            Q_EMIT q_ptr->pasteAvailableChanged();
        }
    }

    bool ClipboardEditScenario::pasteAvailable() const {
        Q_D(const ClipboardEditScenario);
        return d->pasteAvailable;
    }

    bool ClipboardEditScenario::copy(int playheadPosition) const {
        Q_D(const ClipboardEditScenario);
        if (const auto data = d->document->copySelection(playheadPosition)) {
            d->clipboard->copy({*data}, nullptr);
            return true;
        }
        return false;
    }

    void ClipboardEditScenario::cut(int playheadPosition) const {
        const QPointer<const ClipboardEditScenario> scenario(this);
        if (copy(playheadPosition) && scenario) {
            deleteSelection();
        }
    }

    void ClipboardEditScenario::paste(int playheadPosition, Document::PasteMode mode, QQuickWindow *window) const {
        Q_D(const ClipboardEditScenario);
        const QPointer<Document> document(d->document);
        const QPointer<const ClipboardEditScenario> scenario(this);
        const QPointer<QQuickWindow> dialogWindow(window);
        const auto operationValid = [&] { return document && scenario && (!window || dialogWindow); };
        const auto types = d->clipboard->availablePasteTypes();
        if (!operationValid()) {
            return;
        }
        const auto target = document->resolvePasteTarget(types, mode);
        if (!target) {
            return;
        }
        const auto type = target->type();
        bool ok = false;
        const auto data = d->clipboard->paste(type, &ok);
        if (!operationValid()) {
            return;
        }
        if (!ok) {
            d->strategy->reportPasteFailure(window);
            return;
        }
        if (!document->isPasteTargetValid(*target)) {
            return;
        }
        const int position = document->pastePosition(data, playheadPosition, *target);
        if (type == ClipboardData::FreeParameter) {
            Core::TransactionController::ScopedReadLock readLock(document->transactionController());
            if (!operationValid()) {
                return;
            }
            if (!readLock) {
                d->strategy->reportOperationConflict();
                return;
            }
            if (!document->isPasteTargetValid(*target) ||
                !d->resampling->confirmPaste(dialogWindow, document->pasteRequiresResampling(data, position)) || !readLock) {
                return;
            }
        }
        if (!operationValid() || !document->isPasteTargetValid(*target)) {
            return;
        }
        std::optional<Document::PasteResult> result;
        document->transactionController()->beginScopedTransaction(pasteTransactionName(type), [&] {
            if (operationValid() && document->pastePosition(data, playheadPosition, *target) == position) {
                result = document->paste(data, position, *target);
            }
            return result.has_value();
        }, [strategy = QPointer(d->strategy)] {
            if (strategy) {
                strategy->reportOperationConflict();
            }
        });
        if (result && document && scenario) {
            document->selectPasteResult(*result, *target);
        }
    }

    bool ClipboardEditScenario::deleteSelection() const {
        Q_D(const ClipboardEditScenario);
        const QPointer<Document> document(d->document);
        if (!document->anyItemsSelected()) {
            return false;
        }
        const bool freeParameter = document->selectionModel()->selectionType() == dspx::SelectionModel::ST_FreeParameter;
        bool changed = false;
        document->transactionController()->beginScopedTransaction(deleteTransactionName(document), [&] {
            changed = document->deleteSelection();
            return changed;
        }, [strategy = d->strategy] {
            strategy->reportOperationConflict();
        });
        if (changed && document && freeParameter) {
            document->selectionModel()->setFreeParameterRange(0, 0);
        }
        return changed;
    }

    QString ClipboardEditScenario::pasteTransactionName(ClipboardData::Type type) {
        switch (type) {
            case ClipboardData::Tempo:
                return tr("Pasting tempo");
            case ClipboardData::Label:
                return tr("Pasting label");
            case ClipboardData::KeySignature:
                return tr("Pasting key signature");
            case ClipboardData::Track:
                return tr("Pasting track");
            case ClipboardData::Clip:
                return tr("Pasting clip");
            case ClipboardData::Note:
                return tr("Pasting note");
            case ClipboardData::DynamicMixingAnchor:
                return tr("Pasting voice blending anchor");
            case ClipboardData::AnchorNode:
                return tr("Pasting anchor node");
            case ClipboardData::FreeParameter:
                return tr("Pasting parameter data");
        }
        Q_UNREACHABLE_RETURN({});
    }

    QString ClipboardEditScenario::deleteTransactionName(const Document *document) {
        switch (document->selectionModel()->selectionType()) {
            case dspx::SelectionModel::ST_FreeParameter:
                return tr("Deleting parameter data");
            case dspx::SelectionModel::ST_Tempo:
                return tr("Deleting tempo");
            case dspx::SelectionModel::ST_Label:
                return tr("Deleting label");
            case dspx::SelectionModel::ST_KeySignature:
                return tr("Deleting key signature");
            case dspx::SelectionModel::ST_Track:
                return tr("Deleting track");
            case dspx::SelectionModel::ST_Clip:
                return tr("Deleting clip");
            case dspx::SelectionModel::ST_Note:
                return tr("Deleting note");
            case dspx::SelectionModel::ST_AnchorNode:
                return tr("Deleting anchor node");
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                return tr("Deleting voice blending anchor");
            default:
                return tr("Deleting selection");
        }
    }

}

#include "moc_ClipboardEditScenario.cpp"
