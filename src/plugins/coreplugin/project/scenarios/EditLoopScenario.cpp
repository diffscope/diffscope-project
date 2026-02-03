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
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditLoopScenario, "diffscope.core.editloopscenario")

    EditLoopScenario::EditLoopScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new EditLoopScenarioPrivate) {
        Q_D(EditLoopScenario);
        d->q_ptr = this;
    }

    EditLoopScenario::~EditLoopScenario() = default;

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

    void EditLoopScenario::editLoop() const {
        Q_D(const EditLoopScenario);
        if (!d->projectTimeline || !document() || !window())
            return;

        auto timeline = document()->model()->timeline();
        auto startPosition = timeline->loopStart();
        auto endPosition = startPosition + timeline->loopLength();
        auto loopEnabled = timeline->isLoopEnabled();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditLoopDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"timeline", QVariant::fromValue(d->projectTimeline->musicTimeline())},
            {"loopEnabled", loopEnabled},
            {"startPosition", startPosition},
            {"endPosition", endPosition},
        });
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
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

        document()->transactionController()->beginScopedTransaction(tr("Editing loop"), [=] {
            auto dspxTimeline = document()->model()->timeline();
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
