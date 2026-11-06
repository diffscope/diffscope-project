// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditKeySignatureScenario.h"
#include "EditKeySignatureScenario_p.h"

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditKeySignatureStrategy.h>
#include <dspxproject/KeySignatureAtSpecifiedPositionHelper.h>
#include <dspxproject/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcEditKeySignatureScenario, "diffscope.dspxproject.editkeysignaturescenario")

    namespace {

        dspx::KeySignature *keySignatureAt(dspx::KeySignatureSequence *sequence, int position) {
            KeySignatureAtSpecifiedPositionHelper helper;
            helper.setKeySignatureSequence(sequence);
            helper.setPosition(position);
            return helper.keySignature();
        }

    }

    EditKeySignatureScenario::EditKeySignatureScenario(Document *document, EditKeySignatureStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new EditKeySignatureScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy}) {
    }

    EditKeySignatureScenario::~EditKeySignatureScenario() = default;

    void EditKeySignatureScenario::editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        modifyExistingKeySignatureAt(window, timeline, timeline->position(), hints);
    }

    void EditKeySignatureScenario::editKeySignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int tonality, int mode, int accidentalType, DocumentEdit::Hints hints) const {
        Q_D(const EditKeySignatureScenario);
        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcEditKeySignatureScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        const auto result = d->strategy->requestKeySignature({position, doInsertNew, tonality, mode, accidentalType}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument || result->position < 0 ||
            result->tonality < 0 || result->tonality >= 12 || result->mode < 0 || result->mode >= 4096 ||
            (result->accidentalType != dspx::KeySignature::Flat && result->accidentalType != dspx::KeySignature::Sharp))
            return;
        doInsertNew = result->insert;
        tonality = result->tonality;
        mode = result->mode;
        accidentalType = result->accidentalType;
        position = result->position;
        qCInfo(lcEditKeySignatureScenario) << "Edit key signature" << position << tonality << mode << accidentalType;
        if (!doInsertNew) {
            auto keySignatureSequence = d->document->model()->keySignatures();
            auto nearestKeySignature = keySignatureAt(keySignatureSequence, position);
            if (nearestKeySignature) {
                position = nearestKeySignature->position();
                qCInfo(lcEditKeySignatureScenario) << "modify existing key signature at" << position;
            }
        }
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Editing key signature"), [=] {
            auto keySignatureSequence = d->document->model()->keySignatures();
            auto currentKeySignatures = keySignatureSequence->slice(position, 1);
            dspx::KeySignature *keySignatureItem;
            if (currentKeySignatures.isEmpty()) {
                qCDebug(lcEditKeySignatureScenario) << "Current key signatures is empty";
                keySignatureItem = d->document->model()->createKeySignature();
                keySignatureItem->setPosition(position);
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
                if (!keySignatureSequence->insertItem(keySignatureItem)) {
                    d->document->model()->destroyItem(keySignatureItem);
                    return false;
                }
            } else if (currentKeySignatures.size() == 1) {
                qCDebug(lcEditKeySignatureScenario) << "Currently one key signature exists";
                keySignatureItem = currentKeySignatures.first();
                if (keySignatureItem->tonality() == tonality && keySignatureItem->mode() == mode &&
                    keySignatureItem->accidentalType() == static_cast<dspx::KeySignature::AccidentalType>(accidentalType)) {
                    return false;
                }
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
            } else {
                qCWarning(lcEditKeySignatureScenario) << "Unexpected multiple key signatures" << currentKeySignatures.size() << "will be auto removed";
                keySignatureItem = currentKeySignatures.first();
                for (auto redundantKeySignatureItem : currentKeySignatures) {
                    if (redundantKeySignatureItem != keySignatureItem) {
                        keySignatureSequence->removeItem(redundantKeySignatureItem);
                        d->document->model()->destroyItem(redundantKeySignatureItem);
                    }
                }
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
            }
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcEditKeySignatureScenario) << "Failed to edit key signature because the transaction could not start.";
            strategy->reportOperationConflict();
        });
    }

    void EditKeySignatureScenario::insertKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        Q_D(const EditKeySignatureScenario);
        auto keySignatureSequence = d->document->model()->keySignatures();
        auto currentKeySignature = keySignatureAt(keySignatureSequence, position);
        int tonality = 0;
        int mode = 2741;
        int accidentalType = 0;
        if (currentKeySignature) {
            tonality = currentKeySignature->tonality();
            mode = currentKeySignature->mode();
            accidentalType = static_cast<int>(currentKeySignature->accidentalType());
        }
        editKeySignature(window, timeline, position, true, tonality, mode, accidentalType, hints);
    }

    void EditKeySignatureScenario::modifyExistingKeySignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        Q_D(const EditKeySignatureScenario);
        auto keySignatureSequence = d->document->model()->keySignatures();
        auto currentKeySignature = keySignatureAt(keySignatureSequence, position);
        int tonality = 0;
        int mode = 2741;
        int accidentalType = 0;
        if (currentKeySignature) {
            tonality = currentKeySignature->tonality();
            mode = currentKeySignature->mode();
            accidentalType = static_cast<int>(currentKeySignature->accidentalType());
        }
        editKeySignature(window, timeline, position, false, tonality, mode, accidentalType, hints);
    }

}

#include "moc_EditKeySignatureScenario.cpp"
