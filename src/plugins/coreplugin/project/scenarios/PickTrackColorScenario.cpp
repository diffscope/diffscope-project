#include "PickTrackColorScenario.h"
#include "PickTrackColorScenario_p.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Track.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcPickTrackColorScenario, "diffscope.core.picktrackcolorscenario")

    PickTrackColorScenario::PickTrackColorScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new PickTrackColorScenarioPrivate) {
        Q_D(PickTrackColorScenario);
        d->q_ptr = this;
    }

    PickTrackColorScenario::~PickTrackColorScenario() = default;

    void PickTrackColorScenario::pickTrackColor(dspx::Track *track) const {
        Q_D(const PickTrackColorScenario);
        if (!track || !document() || !window())
            return;

        auto currentColorId = track->colorId();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PickTrackColorDialog");
        auto dialog = createAndPositionDialog(&component, {
            {"colorId", currentColorId}
        });
        
        if (!DocumentEditScenarioPrivate::execDialog(dialog))
            return;

        auto newColorId = dialog->property("colorId").toInt();

        qCInfo(lcPickTrackColorScenario) << "Pick track color" << track << "from" << currentColorId << "to" << newColorId;

        document()->transactionController()->beginScopedTransaction(tr("Picking track color"), [=] {
            track->setColorId(newColorId);
            return true;
        }, [] {
            qCCritical(lcPickTrackColorScenario) << "Failed to pick track color in exclusive transaction";
        });
    }

}

#include "moc_PickTrackColorScenario.cpp"