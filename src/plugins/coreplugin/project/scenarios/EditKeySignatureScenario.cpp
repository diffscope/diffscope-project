#include "EditKeySignatureScenario.h"
#include "EditKeySignatureScenario_p.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QCursor>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/KeySignature.h>
#include <dspxmodel/KeySignatureSequence.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditKeySignatureScenario, "diffscope.core.editkeysignaturescenario")

    EditKeySignatureScenario::EditKeySignatureScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new EditKeySignatureScenarioPrivate) {
        Q_D(EditKeySignatureScenario);
        d->q_ptr = this;
    }

    EditKeySignatureScenario::~EditKeySignatureScenario() = default;

    ProjectTimeline *EditKeySignatureScenario::projectTimeline() const {
        Q_D(const EditKeySignatureScenario);
        return d->projectTimeline;
    }

    void EditKeySignatureScenario::setProjectTimeline(ProjectTimeline *projectTimeline) {
        Q_D(EditKeySignatureScenario);
        if (d->projectTimeline != projectTimeline) {
            d->projectTimeline = projectTimeline;
            Q_EMIT projectTimelineChanged();
        }
    }

    void EditKeySignatureScenario::editKeySignature() const {
        Q_D(const EditKeySignatureScenario);
        if (!d->projectTimeline)
            return;
        modifyExistingKeySignatureAt(d->projectTimeline->position());
    }

    void EditKeySignatureScenario::editKeySignature(int position, bool doInsertNew, int tonality, int mode, int accidentalType) const {
        Q_D(const EditKeySignatureScenario);
        if (!d->projectTimeline || !document() || !window())
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditKeySignatureDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"timeline", QVariant::fromValue(d->projectTimeline->musicTimeline())},
            {"position", position},
            {"doInsertNew", doInsertNew},
        });
        dialog->setProperty("tonality", tonality);
        dialog->setProperty("mode", mode);
        dialog->setProperty("accidentalType", accidentalType);
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;
        doInsertNew = dialog->property("doInsertNew").toBool();
        tonality = dialog->property("tonality").toInt();
        mode = dialog->property("mode").toInt();
        accidentalType = dialog->property("accidentalType").toInt();
        position = dialog->property("position").toInt();
        qCInfo(lcEditKeySignatureScenario) << "Edit key signature" << position << tonality << mode << accidentalType;
        if (!doInsertNew) {
            auto keySignatureSequence = document()->model()->timeline()->keySignatures();
            auto nearestKeySignature = keySignatureSequence->itemAt(position);
            if (nearestKeySignature) {
                position = nearestKeySignature->pos();
                qCInfo(lcEditKeySignatureScenario) << "modify existing key signature at" << position;
            }
        }
        document()->transactionController()->beginScopedTransaction(tr("Editing key signature"), [=] {
            auto keySignatureSequence = document()->model()->timeline()->keySignatures();
            auto currentKeySignatures = keySignatureSequence->slice(position, 1);
            dspx::KeySignature *keySignatureItem;
            if (currentKeySignatures.isEmpty()) {
                qCDebug(lcEditKeySignatureScenario) << "Current key signatures is empty";
                keySignatureItem = document()->model()->createKeySignature();
                keySignatureItem->setPos(position);
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
                keySignatureSequence->insertItem(keySignatureItem);
            } else if (currentKeySignatures.size() == 1) {
                qCDebug(lcEditKeySignatureScenario) << "Currently one key signature exists";
                keySignatureItem = currentKeySignatures.first();
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
            } else {
                qCWarning(lcEditKeySignatureScenario) << "Unexpected multiple key signatures" << currentKeySignatures.size() << "will be auto removed";
                keySignatureItem = currentKeySignatures.first();
                for (auto redundantKeySignatureItem : currentKeySignatures) {
                    if (redundantKeySignatureItem != keySignatureItem) {
                        keySignatureSequence->removeItem(redundantKeySignatureItem);
                        document()->model()->destroyItem(redundantKeySignatureItem);
                    }
                }
                keySignatureItem->setTonality(tonality);
                keySignatureItem->setMode(mode);
                keySignatureItem->setAccidentalType(static_cast<dspx::KeySignature::AccidentalType>(accidentalType));
            }
            return true;
        }, [] {
            qCCritical(lcEditKeySignatureScenario) << "Failed to edit key signature in exclusive transaction";
        });
    }

    void EditKeySignatureScenario::insertKeySignatureAt(int position) const {
        Q_D(const EditKeySignatureScenario);
        if (!d->projectTimeline)
            return;
        auto keySignatureSequence = document()->model()->timeline()->keySignatures();
        auto currentKeySignature = keySignatureSequence->itemAt(position);
        int tonality = 0;
        int mode = 2741;
        int accidentalType = 0;
        if (currentKeySignature) {
            tonality = currentKeySignature->tonality();
            mode = currentKeySignature->mode();
            accidentalType = static_cast<int>(currentKeySignature->accidentalType());
        }
        editKeySignature(position, true, tonality, mode, accidentalType);
    }

    void EditKeySignatureScenario::modifyExistingKeySignatureAt(int position) const {
        Q_D(const EditKeySignatureScenario);
        if (!d->projectTimeline)
            return;
        auto keySignatureSequence = document()->model()->timeline()->keySignatures();
        auto currentKeySignature = keySignatureSequence->itemAt(position);
        int tonality = 0;
        int mode = 2741;
        int accidentalType = 0;
        if (currentKeySignature) {
            tonality = currentKeySignature->tonality();
            mode = currentKeySignature->mode();
            accidentalType = static_cast<int>(currentKeySignature->accidentalType());
        }
        editKeySignature(position, false, tonality, mode, accidentalType);
    }

}

#include "moc_EditKeySignatureScenario.cpp"
