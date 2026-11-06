// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditTempoTimeSignatureScenario.h"
#include "EditTempoTimeSignatureScenario_p.h"

#include <bit>

#include <QLoggingCategory>
#include <QPointer>
#include <QQuickWindow>

#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTime.h>

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>

#include <dspxproject/Document.h>
#include <dspxproject/EditTempoTimeSignatureStrategy.h>
#include <dspxproject/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcEditTempoTimeSignatureScenario, "diffscope.dspxproject.edittempotimesignaturescenario")

    EditTempoTimeSignatureScenario::EditTempoTimeSignatureScenario(Document *document, EditTempoTimeSignatureStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new EditTempoTimeSignatureScenarioPrivate{.q_ptr = this, .document = document, .strategy = strategy}) {
    }

    EditTempoTimeSignatureScenario::~EditTempoTimeSignatureScenario() = default;

    void EditTempoTimeSignatureScenario::editTempo(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        modifyExistingTempoAt(window, timeline, timeline->position(), hints);
    }
    void EditTempoTimeSignatureScenario::editTempo(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, double tempo, DocumentEdit::Hints hints) const {
        Q_D(const EditTempoTimeSignatureScenario);
        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        const QPointer<ProjectTimeline> targetTimeline(timeline);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcEditTempoTimeSignatureScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        const auto result = d->strategy->requestTempo({position, doInsertNew, tempo}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument || !targetTimeline ||
            result->position < 0 || !(result->tempo >= 10 && result->tempo <= 1000))
            return;
        doInsertNew = result->insert;
        tempo = result->tempo;
        position = result->position;
        qCInfo(lcEditTempoTimeSignatureScenario) << "Edit tempo" << position << tempo;
        if (!doInsertNew) {
            position = targetTimeline->musicTimeline()->nearestTickWithTempoTo(position);
            qCInfo(lcEditTempoTimeSignatureScenario) << "modify existing tempo at" << position;
        }
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Editing tempo"), [=] {
            auto tempoSequence = d->document->model()->tempos();
            auto currentTempos = tempoSequence->slice(position, 1);
            dspx::Tempo *tempoItem;
            if (currentTempos.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Current tempos is empty";
                tempoItem = d->document->model()->createTempo();
                tempoItem->setPosition(position);
                tempoItem->setValue(tempo);
                if (!tempoSequence->insertItem(tempoItem)) {
                    d->document->model()->destroyItem(tempoItem);
                    return false;
                }
            } else if (currentTempos.size() == 1) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Currently one tempo exists";
                tempoItem = currentTempos.first();
                if (tempoItem->value() == tempo) {
                    return false;
                }
                tempoItem->setValue(tempo);
            } else {
                qCWarning(lcEditTempoTimeSignatureScenario) << "Unexpected multiple tempos" << currentTempos.size() << "will be auto removed";
                tempoItem = currentTempos.first();
                for (auto redundantTempoItem : currentTempos) {
                    if (redundantTempoItem != tempoItem) {
                        tempoSequence->removeItem(redundantTempoItem);
                        d->document->model()->destroyItem(redundantTempoItem);
                    }
                }
                tempoItem->setValue(tempo);
            }
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcEditTempoTimeSignatureScenario) << "Failed to edit tempo because the transaction could not start.";
            strategy->reportOperationConflict();
        });

    }
    void EditTempoTimeSignatureScenario::insertTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        editTempo(window, timeline, position, true, timeline->musicTimeline()->tempoAt(position), hints);
    }
    void EditTempoTimeSignatureScenario::modifyExistingTempoAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        editTempo(window, timeline, position, false, timeline->musicTimeline()->tempoAt(position), hints);
    }
    void EditTempoTimeSignatureScenario::editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, DocumentEdit::Hints hints) const {
        modifyExistingTimeSignatureAt(window, timeline, timeline->position(), hints);
    }
    void EditTempoTimeSignatureScenario::editTimeSignature(QQuickWindow *window, ProjectTimeline *timeline, int position, bool doInsertNew, int numerator, int denominator, DocumentEdit::Hints hints) const {
        Q_D(const EditTempoTimeSignatureScenario);
        const QPointer<Document> targetDocument(d->document);
        const QPointer<QQuickWindow> dialogWindow(window);
        const QPointer<ProjectTimeline> targetTimeline(timeline);
        Core::TransactionController::ScopedReadLock readLock(targetDocument->transactionController());
        if (!readLock) {
            qCWarning(lcEditTempoTimeSignatureScenario) << "Could not acquire a document read lock.";
            d->strategy->reportOperationConflict();
            return;
        }

        const auto result = d->strategy->requestTimeSignature({position, doInsertNew, numerator, denominator}, timeline->musicTimeline(), dialogWindow, hints);
        if (!result || !readLock || !targetDocument || !targetTimeline ||
            result->position < 0 || result->numerator <= 0 || result->denominator > 128 ||
            !std::has_single_bit(static_cast<unsigned int>(result->denominator)))
            return;
        doInsertNew = result->insert;
        numerator = result->numerator;
        denominator = result->denominator;
        int measure = targetTimeline->musicTimeline()->create(0, 0, result->position).measure();
        qCInfo(lcEditTempoTimeSignatureScenario) << "Edit time signature" << measure << numerator << denominator;
        if (!doInsertNew) {
            measure = targetTimeline->musicTimeline()->nearestBarWithTimeSignatureTo(measure);
            qCInfo(lcEditTempoTimeSignatureScenario) << "modify existing time signature at" << measure;
        }
        readLock.release();
        if (!targetDocument)
            return;
        targetDocument->transactionController()->beginScopedTransaction(tr("Editing time signature"), [=] {
            auto timeSignatureSequence = d->document->model()->timeSignatures();
            auto currentTimeSignatures = timeSignatureSequence->slice(measure, 1);
            dspx::TimeSignature *timeSignatureItem;
            if (currentTimeSignatures.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Current time signatures is empty";
                timeSignatureItem = d->document->model()->createTimeSignature();
                timeSignatureItem->setIndex(measure);
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
                if (!timeSignatureSequence->insertItem(timeSignatureItem)) {
                    d->document->model()->destroyItem(timeSignatureItem);
                    return false;
                }
            } else if (currentTimeSignatures.size() == 1) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Currently one time signature exists";
                timeSignatureItem = currentTimeSignatures.first();
                if (timeSignatureItem->numerator() == numerator && timeSignatureItem->denominator() == denominator) {
                    return false;
                }
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
            } else {
                qCWarning(lcEditTempoTimeSignatureScenario) << "Unexpected multiple time signatures" << currentTimeSignatures.size() << "will be auto removed";
                timeSignatureItem = currentTimeSignatures.first();
                for (auto redundantTimeSignatureItem : currentTimeSignatures) {
                    if (redundantTimeSignatureItem != timeSignatureItem) {
                        timeSignatureSequence->removeItem(redundantTimeSignatureItem);
                        d->document->model()->destroyItem(redundantTimeSignatureItem);
                    }
                }
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
            }
            return true;
        }, [strategy = d->strategy] {
            qCWarning(lcEditTempoTimeSignatureScenario) << "Failed to edit the time signature because the transaction could not start.";
            strategy->reportOperationConflict();
        });
    }
    void EditTempoTimeSignatureScenario::insertTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        auto timeSignature = timeline->musicTimeline()->timeSignatureAt(timeline->musicTimeline()->create(0, 0, position).measure());
        editTimeSignature(window, timeline, position, true, timeSignature.numerator(), timeSignature.denominator(), hints);
    }
    void EditTempoTimeSignatureScenario::modifyExistingTimeSignatureAt(QQuickWindow *window, ProjectTimeline *timeline, int position, DocumentEdit::Hints hints) const {
        auto timeSignature = timeline->musicTimeline()->timeSignatureAt(timeline->musicTimeline()->create(0, 0, position).measure());
        editTimeSignature(window, timeline, position, false, timeSignature.numerator(), timeSignature.denominator(), hints);
    }

}

#include "moc_EditTempoTimeSignatureScenario.cpp"
