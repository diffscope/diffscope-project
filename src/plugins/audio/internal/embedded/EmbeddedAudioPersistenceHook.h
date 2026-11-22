// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EMBEDDEDAUDIOPERSISTENCEHOOK_H
#define DIFFSCOPE_AUDIO_EMBEDDEDAUDIOPERSISTENCEHOOK_H

#include <dspxproject/IDocumentPersistenceHook.h>

namespace Audio::Internal {

    class EmbeddedAudioPersistenceHook : public DspxProject::IDocumentPersistenceHook {
    public:
        using IDocumentPersistenceHook::IDocumentPersistenceHook;

        void beforeLoadModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) override;
        void afterSaveModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model) override;
    };

}

#endif // DIFFSCOPE_AUDIO_EMBEDDEDAUDIOPERSISTENCEHOOK_H
