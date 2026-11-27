// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTEDITORPERSISTENCEHOOK_H
#define DIFFSCOPE_COREPLUGIN_PROJECTEDITORPERSISTENCEHOOK_H

#include <dspxproject/IDocumentPersistenceHook.h>

namespace Core::Internal {

    class ProjectEditorPersistenceHook : public DspxProject::IDocumentPersistenceHook {
    public:
        using IDocumentPersistenceHook::IDocumentPersistenceHook;

        void beforeLoadModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) override;
        void afterSaveModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model, const QString &destination) override;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTEDITORPERSISTENCEHOOK_H
