// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_DOCUMENTEDITDIALOG_H
#define DIFFSCOPE_COREPLUGIN_DOCUMENTEDITDIALOG_H

#include <memory>

#include <QVariantMap>

#include <uishell/DialogPositioner.h>
#include <dspxproject/DocumentEditHints.h>

namespace Core::Internal::DocumentEditDialog {

    UIShell::DialogPositioner::Placement placementFor(DspxProject::DocumentEdit::Hints hints);
    std::unique_ptr<QObject> createPopup(const char *type, QQuickWindow *window, UIShell::DialogPositioner::Placement placement, const QVariantMap &initialProperties);
    bool execPopup(QObject *popup, QQuickWindow *window);

}

#endif // DIFFSCOPE_COREPLUGIN_DOCUMENTEDITDIALOG_H
