#include "EditTempoTimeSignatureScenario.h"
#include "EditTempoTimeSignatureScenario_p.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QCursor>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTime.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignature.h>
#include <dspxmodel/TimeSignatureSequence.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditTempoTimeSignatureScenario, "diffscope.core.edittempotimesignaturescenario")

    EditTempoTimeSignatureScenario::EditTempoTimeSignatureScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new EditTempoTimeSignatureScenarioPrivate) {
        Q_D(EditTempoTimeSignatureScenario);
        d->q_ptr = this;
    }

    EditTempoTimeSignatureScenario::~EditTempoTimeSignatureScenario() = default;

    ProjectTimeline *EditTempoTimeSignatureScenario::projectTimeline() const {
        Q_D(const EditTempoTimeSignatureScenario);
        return d->projectTimeline;
    }

    void EditTempoTimeSignatureScenario::setProjectTimeline(ProjectTimeline *projectTimeline) {
        Q_D(EditTempoTimeSignatureScenario);
        if (d->projectTimeline != projectTimeline) {
            d->projectTimeline = projectTimeline;
            Q_EMIT projectTimelineChanged();
        }
    }

    void EditTempoTimeSignatureScenario::editTempo() const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        modifyExistingTempoAt(d->projectTimeline->position());
    }
    void EditTempoTimeSignatureScenario::editTempo(int position, bool doInsertNew, double tempo) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline || !document() || !window())
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTempoDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"timeline", QVariant::fromValue(d->projectTimeline->musicTimeline())},
            {"position", position},
            {"doInsertNew", doInsertNew},
        });
        dialog->setProperty("tempo", tempo);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;
        doInsertNew = dialog->property("doInsertNew").toBool();
        tempo = dialog->property("tempo").toDouble();
        position = dialog->property("position").toInt();
        qCInfo(lcEditTempoTimeSignatureScenario) << "Edit tempo" << position << tempo;
        if (!doInsertNew) {
            position = d->projectTimeline->musicTimeline()->nearestTickWithTempoTo(position);
            qCInfo(lcEditTempoTimeSignatureScenario) << "modify existing tempo at" << position;
        }
        document()->transactionController()->beginScopedTransaction(tr("Editing tempo"), [=] {
            auto tempoSequence = document()->model()->timeline()->tempos();
            auto currentTempos = tempoSequence->slice(position, 1);
            dspx::Tempo *tempoItem;
            if (currentTempos.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Current tempos is empty";
                tempoItem = document()->model()->createTempo();
                tempoItem->setPos(position);
                tempoItem->setValue(tempo);
                tempoSequence->insertItem(tempoItem);
            } else if (currentTempos.size() == 1) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Currently one tempo exists";
                tempoItem = currentTempos.first();
                tempoItem->setValue(tempo);
            } else {
                qCWarning(lcEditTempoTimeSignatureScenario) << "Unexpected multiple tempos" << currentTempos.size() << "will be auto removed";
                tempoItem = currentTempos.first();
                for (auto redundantTempoItem : currentTempos) {
                    if (redundantTempoItem != tempoItem) {
                        tempoSequence->removeItem(redundantTempoItem);
                        document()->model()->destroyItem(redundantTempoItem);
                    }
                }
                tempoItem->setValue(tempo);
            }
            return true;
        }, [] {
            qCCritical(lcEditTempoTimeSignatureScenario) << "Failed to edit tempo in exclusive transaction";
        });


    }
    void EditTempoTimeSignatureScenario::insertTempoAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        editTempo(position, true, d->projectTimeline->musicTimeline()->tempoAt(position));
    }
    void EditTempoTimeSignatureScenario::modifyExistingTempoAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        editTempo(position, false, d->projectTimeline->musicTimeline()->tempoAt(position));
    }
    void EditTempoTimeSignatureScenario::editTimeSignature() const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        modifyExistingTimeSignatureAt(d->projectTimeline->position());
    }
    void EditTempoTimeSignatureScenario::editTimeSignature(int position, bool doInsertNew, int numerator, int denominator) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline || !document() || !window())
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTimeSignatureDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"timeline", QVariant::fromValue(d->projectTimeline->musicTimeline())},
            {"position", position},
            {"doInsertNew", doInsertNew},
        });
        dialog->setProperty("numerator", numerator);
        dialog->setProperty("denominator", denominator);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;
        doInsertNew = dialog->property("doInsertNew").toBool();
        numerator = dialog->property("numerator").toInt();
        denominator = dialog->property("denominator").toInt();
        int measure = d->projectTimeline->musicTimeline()->create(0, 0, position).measure();
        qCInfo(lcEditTempoTimeSignatureScenario) << "Edit time signature" << measure << numerator << denominator;
        if (!doInsertNew) {
            measure = d->projectTimeline->musicTimeline()->nearestBarWithTimeSignatureTo(measure);
            qCInfo(lcEditTempoTimeSignatureScenario) << "modify existing time signature at" << measure;
        }
        document()->transactionController()->beginScopedTransaction(tr("Editing time signature"), [=] {
            auto timeSignatureSequence = document()->model()->timeline()->timeSignatures();
            auto currentTimeSignatures = timeSignatureSequence->slice(measure, 1);
            dspx::TimeSignature *timeSignatureItem;
            if (currentTimeSignatures.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Current time signatures is empty";
                timeSignatureItem = document()->model()->createTimeSignature();
                timeSignatureItem->setIndex(measure);
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
                timeSignatureSequence->insertItem(timeSignatureItem);
            } else if (currentTimeSignatures.size() == 1) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Currently one time signature exists";
                timeSignatureItem = currentTimeSignatures.first();
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
            } else {
                qCWarning(lcEditTempoTimeSignatureScenario) << "Unexpected multiple time signatures" << currentTimeSignatures.size() << "will be auto removed";
                timeSignatureItem = currentTimeSignatures.first();
                for (auto redundantTimeSignatureItem : currentTimeSignatures) {
                    if (redundantTimeSignatureItem != timeSignatureItem) {
                        timeSignatureSequence->removeItem(redundantTimeSignatureItem);
                        document()->model()->destroyItem(redundantTimeSignatureItem);
                    }
                }
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
            }
            return true;
        }, [] {
            qCCritical(lcEditTempoTimeSignatureScenario) << "Failed to edit tempo in exclusive transaction";
        });
    }
    void EditTempoTimeSignatureScenario::insertTimeSignatureAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        auto timeSignature = d->projectTimeline->musicTimeline()->timeSignatureAt(d->projectTimeline->musicTimeline()->create(0, 0, position).measure());
        editTimeSignature(position, true, timeSignature.numerator(), timeSignature.denominator());
    }
    void EditTempoTimeSignatureScenario::modifyExistingTimeSignatureAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        auto timeSignature = d->projectTimeline->musicTimeline()->timeSignatureAt(d->projectTimeline->musicTimeline()->create(0, 0, position).measure());
        editTimeSignature(position, false, timeSignature.numerator(), timeSignature.denominator());
    }

}

#include "moc_EditTempoTimeSignatureScenario.cpp"
