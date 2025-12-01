#include "EditTempoTimeSignatureScenario.h"
#include "EditTempoTimeSignatureScenario_p.h"

#include <QQuickWindow>
#include <QQmlComponent>
#include <QEventLoop>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTimeSignature.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/DspxDocument.h>

namespace Core {

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
        dialog->setProperty("x", window->width() / 2.0 - width / 2);
        if (auto popupTopMarginHint = window->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
            // Assume it is project window
            dialog->setProperty("y", popupTopMarginHint);
        } else {
            dialog->setProperty("y", window->height() / 2.0 - height / 2);
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
    void EditTempoTimeSignatureScenario::editTempo() const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline)
            return;
        modifyExistingTempoAt(d->projectTimeline->position());
    }
    void EditTempoTimeSignatureScenario::editTempo(int position, bool doInsertNew, double initialTempo) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline || !d->document || !d->window)
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTempoDialog");
        auto dialog = d->createAndPositionDialog(&component, position, doInsertNew);
        dialog->setProperty("tempo", initialTempo);
        if (!d->execDialog(dialog))
            return;
        qDebug() << "Tempo" << dialog->property("tempo");
        // TODO

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
    void EditTempoTimeSignatureScenario::editTimeSignature(int position, bool doInsertNew, int initialNumerator, int initialDenominator) const {
        Q_D(const EditTempoTimeSignatureScenario);
        if (!d->projectTimeline || !d->document || !d->window)
            return;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditTimeSignatureDialog");
        auto dialog = d->createAndPositionDialog(&component, position, doInsertNew);
        dialog->setProperty("numerator", initialNumerator);
        dialog->setProperty("denominator", initialDenominator);
        if (!d->execDialog(dialog))
            return;
        qDebug() << "TimeSignature" << dialog->property("numerator") << dialog->property("denominator");
        // TODO
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
