// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectEditorPersistenceHook.h"

#include <application_config.h>

#include <opendspx/model.h>

#include <coreplugin/CoreInterface.h>
#include <stdcorelib/support/json.h>

namespace Core::Internal {

    static void writeEditorMetadata(opendspx::Model &model) {
        model.content.global.editorId = CoreInterface::dspxEditorId();
        model.content.global.editorName = "DiffScope";
        model.content.workspace["diffscope"]["editorVersion"] = APPLICATION_SEMVER;
    }

    void ProjectEditorPersistenceHook::beforeLoadModel(DspxProject::ProjectDocumentContext &, opendspx::Model &model, const QString &) {
        writeEditorMetadata(model);
    }

    void ProjectEditorPersistenceHook::afterSaveModel(DspxProject::ProjectDocumentContext &, opendspx::Model &model) {
        writeEditorMetadata(model);
    }

}
