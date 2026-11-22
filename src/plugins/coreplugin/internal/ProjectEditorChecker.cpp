// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectEditorChecker.h"

#include <application_config.h>

#include <QApplication>
#include <QCoreApplication>
#include <QString>

#include <opendspx/model.h>

#include <SVSCraftCore/Semver.h>

#include <coreplugin/CoreInterface.h>
#include <stdcorelib/support/json.h>

namespace Core::Internal {

    static QString editorVersion(const opendspx::Model &model) {
        const auto workspace = model.content.workspace.find("diffscope");
        if (workspace == model.content.workspace.end()) {
            return {};
        }
        const auto version = workspace->second.find("editorVersion");
        return version == workspace->second.end() ? QString() : QString::fromStdString(version->second.toString());
    }

    QList<DspxProject::CheckWarning> ProjectEditorChecker::runCheck(const opendspx::Model &model, Level, bool) {
        const auto &global = model.content.global;
        if (global.editorId != CoreInterface::dspxEditorId()) {
            const auto name = QString::fromStdString(global.editorName);
            return {{
                .level = Strong,
                .message = QCoreApplication::translate("Core::ProjectFileDialogs", "File created with another application"),
                .jsonPath = QStringLiteral("$.content.global.editorId"),
                .detail = QCoreApplication::translate("Core::ProjectFileDialogs", "This project file was created with another application (%1). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                              .arg(name.isEmpty() ? QCoreApplication::translate("Core::ProjectFileDialogs", "name unknown") : name),
                .info = {
                    {QCoreApplication::translate("Core::DspxInspectorDialog", "Editor ID"), QString::fromStdString(global.editorId)},
                    {QCoreApplication::translate("Core::DspxInspectorDialog", "Editor name"), name},
                },
            }};
        }

        const auto version = editorVersion(model);
        if (version.isEmpty()) {
            return {};
        }
        const SVS::Semver current(QStringLiteral(APPLICATION_SEMVER));
        const SVS::Semver file(version);
        if (file == current || (!(file > current) && file.preRelease().isEmpty() && file.build().isEmpty())) {
            return {};
        }

        const auto applicationName = QApplication::applicationDisplayName();
        return {{
            .level = Strong,
            .message = QCoreApplication::translate("Core::ProjectFileDialogs", "File created with a potentially incompatible %1 version").arg(applicationName),
            .jsonPath = QStringLiteral("$.content.workspace.diffscope.editorVersion"),
            .detail = QCoreApplication::translate("Core::ProjectFileDialogs", "This project file was created with a newer version or test version of %1 (%2). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                          .arg(applicationName, version),
            .info = {{QCoreApplication::translate("Core::DspxInspectorDialog", "Version"), version}},
        }};
    }

}
