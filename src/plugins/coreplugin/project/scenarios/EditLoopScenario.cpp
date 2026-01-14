#include "EditLoopScenario.h"
#include "EditLoopScenario_p.h"

#include <QCursor>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditLoopScenario, "diffscope.core.editloopscenario")

    QObject *EditLoopScenarioPrivate::createAndPositionDialog(QQmlComponent *component, int startPosition, int endPosition, bool loopEnabled) const {
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        QObject *dialog = component->createWithInitialProperties({
            {"parent", QVariant::fromValue(window->contentItem())},
            {"timeline", QVariant::fromValue(projectTimeline->musicTimeline())},
            {"loopEnabled", loopEnabled},
            {"startPosition", startPosition},
            {"endPosition", endPosition},
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
                dialog->setProperty("y", popupTopMarginHint);
            } else {
                dialog->setProperty("y", window->height() / 2.0 - height / 2);
            }
        }
        return dialog;
    }

    bool EditLoopScenarioPrivate::execDialog(QObject *dialog) const {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    EditLoopScenario::EditLoopScenario(QObject *parent)
        : QObject(parent), d_ptr(new EditLoopScenarioPrivate) {
        Q_D(EditLoopScenario);
        d->q_ptr = this;
    }

    EditLoopScenario::~EditLoopScenario() = default;

    QQuickWindow *EditLoopScenario::window() const {
        Q_D(const EditLoopScenario);
        return d->window;
    }

    void EditLoopScenario::setWindow(QQuickWindow *window) {
        Q_D(EditLoopScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

    ProjectTimeline *EditLoopScenario::projectTimeline() const {
        Q_D(const EditLoopScenario);
        return d->projectTimeline;
    }

    void EditLoopScenario::setProjectTimeline(ProjectTimeline *projectTimeline) {
        Q_D(EditLoopScenario);
        if (d->projectTimeline != projectTimeline) {
            d->projectTimeline = projectTimeline;
            Q_EMIT projectTimelineChanged();
        }
    }

    DspxDocument *EditLoopScenario::document() const {
        Q_D(const EditLoopScenario);
        return d->document;
    }

    void EditLoopScenario::setDocument(DspxDocument *document) {
        Q_D(EditLoopScenario);
        if (d->document != document) {
            d->document = document;
            Q_EMIT documentChanged();
        }
    }

    bool EditLoopScenario::shouldDialogPopupAtCursor() const {
        Q_D(const EditLoopScenario);
        return d->shouldDialogPopupAtCursor;
    }

    void EditLoopScenario::setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor) {
        Q_D(EditLoopScenario);
        if (d->shouldDialogPopupAtCursor != shouldDialogPopupAtCursor) {
            d->shouldDialogPopupAtCursor = shouldDialogPopupAtCursor;
            Q_EMIT shouldDialogPopupAtCursorChanged();
        }
    }

    void EditLoopScenario::editLoop() const {
        Q_D(const EditLoopScenario);
        if (!d->projectTimeline || !d->document || !d->window)
            return;

        auto timeline = d->document->model()->timeline();
        auto startPosition = timeline->loopStart();
        auto endPosition = startPosition + timeline->loopLength();
        auto loopEnabled = timeline->isLoopEnabled();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditLoopDialog");
        auto dialog = d->createAndPositionDialog(&component, startPosition, endPosition, loopEnabled);
        if (!d->execDialog(dialog))
            return;

        loopEnabled = dialog->property("loopEnabled").toBool();
        startPosition = dialog->property("startPosition").toInt();
        endPosition = dialog->property("endPosition").toInt();

        if (startPosition < 0) {
            qCWarning(lcEditLoopScenario) << "Invalid loop start" << startPosition;
            return;
        }

        auto loopLength = endPosition - startPosition;
        if (loopLength <= 0) {
            qCWarning(lcEditLoopScenario) << "Invalid loop length" << loopLength << "from" << startPosition << "to" << endPosition;
            return;
        }

        qCInfo(lcEditLoopScenario) << "Edit loop" << loopEnabled << startPosition << loopLength;

        d->document->transactionController()->beginScopedTransaction(tr("Editing loop"), [=] {
            auto dspxTimeline = d->document->model()->timeline();
            dspxTimeline->setLoopEnabled(loopEnabled);
            dspxTimeline->setLoopStart(startPosition);
            dspxTimeline->setLoopLength(loopLength);
            return true;
        }, [] {
            qCCritical(lcEditLoopScenario) << "Failed to edit loop in exclusive transaction";
        });
    }

}

#include "moc_EditLoopScenario.cpp"
