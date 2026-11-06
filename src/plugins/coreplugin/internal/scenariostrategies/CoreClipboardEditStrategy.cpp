// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreClipboardEditStrategy.h"

#include <QCoreApplication>
#include <QQuickWindow>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    CoreClipboardEditStrategy::CoreClipboardEditStrategy(DspxProject::Document *document) : m_document(document) {
    }

    void CoreClipboardEditStrategy::reportPasteFailure(QQuickWindow *window) {
        SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), window,
                                  QCoreApplication::translate("Core::Internal::CoreClipboardEditStrategy", "Paste Failed"),
                                  QCoreApplication::translate("Core::Internal::CoreClipboardEditStrategy", "Cannot paste data from the clipboard."));
    }

    void CoreClipboardEditStrategy::reportOperationConflict() {
        ProjectWindowInterface::of(m_document)->sendOperationConflictNotification();
    }

}
