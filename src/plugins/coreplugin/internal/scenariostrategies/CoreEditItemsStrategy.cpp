// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreEditItemsStrategy.h"

#include <QCoreApplication>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    CoreEditItemsStrategy::CoreEditItemsStrategy(DspxProject::Document *document) : m_document(document) {
    }

    bool CoreEditItemsStrategy::confirmSingerLayoutChange(QQuickWindow *window) {
        return SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(), window,
            QCoreApplication::translate("Core::Internal::CoreEditItemsStrategy", "Warning"),
            QCoreApplication::translate("Core::Internal::CoreEditItemsStrategy", "The selected clips use different source singer layouts. Bouncing will keep one source singer layout and discard all voice blending anchors.\n\nContinue to bounce?"),
            SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No
        ) == SVS::SVSCraft::Yes;
    }

    bool CoreEditItemsStrategy::confirmBounceDataLoss(QQuickWindow *window) {
        return SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(), window,
            QCoreApplication::translate("Core::Internal::CoreEditItemsStrategy", "Warning"),
            QCoreApplication::translate("Core::Internal::CoreEditItemsStrategy", "Some notes or curves cross clip boundaries, or data points overlap after merging. Bouncing may trim or overwrite this data.\n\nContinue to bounce?"),
            SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No
        ) == SVS::SVSCraft::Yes;
    }

    void CoreEditItemsStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
