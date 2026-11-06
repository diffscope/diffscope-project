// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREEDITKEYSIGNATURESTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREEDITKEYSIGNATURESTRATEGY_H

#include <dspxproject/EditKeySignatureStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreEditKeySignatureStrategy : public DspxProject::EditKeySignatureStrategy {
    public:
        explicit CoreEditKeySignatureStrategy(DspxProject::Document *document);

        std::optional<Input> requestKeySignature(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREEDITKEYSIGNATURESTRATEGY_H
