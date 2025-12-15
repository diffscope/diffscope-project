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

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditTempoTimeSignatureScenario, "diffscope.core.edittempotimesignaturescenario")

    QObject *EditTempoTimeSignatureScenarioPrivate::createAndPositionDialog(QQmlComponent *component, int position, bool doInsertNew) const {
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        QObject *dialog = component->createWithInitialProperties({
            {"parent", QVariant::fromValue(window->contentItem())},
            {"timeline", QVariant::fromValue(projectTimeline->musicTimeline())},
            {"position", position},
            {"doInsertNew", doInsertNew},
        });
        if (!dialog) {
            qFatal() << component->errorString();
        }
        auto width = dialog->property("width").toDouble();
        auto height = dialog->property("height").toDouble();
        if (shouldDialogPopupAtCursor) {
            auto pos = window->mapFromGlobal(QCursor::pos()).toPointF();
            dialog->setProperty("x", qBound(0.0, pos.x(), window->width() - width));
            dialog->setProperty("y", qBound(0.0, pos.y(), window->height() - height));
        } else {
            dialog->setProperty("x", window->width() / 2.0 - width / 2);
            if (auto popupTopMarginHint = window->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
                // Assume it is project window
                dialog->setProperty("y", popupTopMarginHint);
            } else {
                dialog->setProperty("y", window->height() / 2.0 - height / 2);
            }
        }
        return dialog;

    }

    bool EditTempoTimeSignatureScenarioPrivate::execDialog(QObject *dialog) const {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    EditTempoTimeSignatureScenario::EditTempoTimeSignatureScenario(QObject *parent)
        : QObject(parent), d_ptr(new EditTempoTimeSignatureScenarioPrivate) {
        Q_D(EditTempoTimeSignatureScenario);
        d->q_ptr = this;
    }

    EditTempoTimeSignatureScenario::~EditTempoTimeSignatureScenario() = default;

    QQuickWindow *EditTempoTimeSignatureScenario::window() const {
        Q_D(const EditTempoTimeSignatureScenario);
        return d->window;
    }

    void EditTempoTimeSignatureScenario::setWindow(QQuickWindow *window) {
        Q_D(EditTempoTimeSignatureScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

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

    DspxDocument *EditTempoTimeSignatureScenario::document() const {
        Q_D(const EditTempoTimeSignatureScenario);
        return d->document;
    }

    void EditTempoTimeSignatureScenario::setDocument(DspxDocument *document) {
        Q_D(EditTempoTimeSignatureScenario);
        if (d->document != document) {
            d->document = document;
            Q_EMIT documentChanged();
        }
    }

    bool EditTempoTimeSignatureScenario::shouldDialogPopupAtCursor() const {
        Q_D(const EditTempoTimeSignatureScenario);
        return d->shouldDialogPopupAtCursor;
    }

    void EditTempoTimeSignatureScenario::setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor) {
        Q_D(EditTempoTimeSignatureScenario);
        if (d->shouldDialogPopupAtCursor != shouldDialogPopupAtCursor) {
            d->shouldDialogPopupAtCursor = shouldDialogPopupAtCursor;
            Q_EMIT shouldDialogPopupAtCursorChanged();
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
        if (!d->projectTimeline || !d->document || !d->window)
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTempoDialog");
        auto dialog = d->createAndPositionDialog(&component, position, doInsertNew);
        dialog->setProperty("tempo", tempo);
        if (!d->execDialog(dialog))
            return;
        doInsertNew = dialog->property("doInsertNew").toBool();
        tempo = dialog->property("tempo").toDouble();
        position = dialog->property("position").toInt();
        qCInfo(lcEditTempoTimeSignatureScenario) << "Edit tempo" << position << tempo;
        if (!doInsertNew) {
            position = d->projectTimeline->musicTimeline()->nearestTickWithTempoTo(position);
            qCInfo(lcEditTempoTimeSignatureScenario) << "modify existing tempo at" << position;
        }
        d->document->transactionController()->beginScopedTransaction(tr("Editing tempo"), [=] {
            auto tempoSequence = d->document->model()->timeline()->tempos();
            auto currentTempos = tempoSequence->slice(position, position + 1);
            dspx::Tempo *tempoItem;
            if (currentTempos.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario) << "Current tempos is empty";
                tempoItem = d->document->model()->createTempo();
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
                        d->document->model()->destroyItem(redundantTempoItem);
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
        if (!d->projectTimeline || !d->document || !d->window)
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTimeSignatureDialog");
        auto dialog = d->createAndPositionDialog(&component, position, doInsertNew);
        dialog->setProperty("numerator", numerator);
        dialog->setProperty("denominator", denominator);
        if (!d->execDialog(dialog))
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
        d->document->transactionController()->beginScopedTransaction(tr("Editing time signature"), [=] {
            auto timeSignatureSequence = d->document->model()->timeline()->timeSignatures();
            auto currentTimeSignatures = timeSignatureSequence->slice(measure, measure + 1);
            dspx::TimeSignature *timeSignatureItem;
            if (currentTimeSignatures.isEmpty()) {
                qCDebug(lcEditTempoTimeSignatureScenario()) << "Current time signatures is empty";
                timeSignatureItem = d->document->model()->createTimeSignature();
                timeSignatureItem->setIndex(measure);
                timeSignatureItem->setNumerator(numerator);
                timeSignatureItem->setDenominator(denominator);
                timeSignatureSequence->insertItem(timeSignatureItem);
            } else if (currentTimeSignatures.size() == 1) {
                qCDebug(lcEditTempoTimeSignatureScenario()) << "Currently one time signature exists";
                timeSignatureItem = currentTimeSignatures.first();
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
        }, [] {
            qCCritical(lcEditTempoTimeSignatureScenario) << "Failed to edit tempo in exclusive transaction";
        });
    }
    void EditTempoTimeSignatureScenario::insertTimeSignatureAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        auto timeSignature = d->projectTimeline->musicTimeline()->timeSignatureAt(position);
        editTimeSignature(position, true, timeSignature.numerator(), timeSignature.denominator());
    }
    void EditTempoTimeSignatureScenario::modifyExistingTimeSignatureAt(int position) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        auto timeSignature = d->projectTimeline->musicTimeline()->timeSignatureAt(position);
        editTimeSignature(position, false, timeSignature.numerator(), timeSignature.denominator());
    }

}

#include "moc_EditTempoTimeSignatureScenario.cpp"
