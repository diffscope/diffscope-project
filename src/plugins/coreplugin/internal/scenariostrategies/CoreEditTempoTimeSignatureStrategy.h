// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREEDITTEMPOTIMESIGNATURESTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREEDITTEMPOTIMESIGNATURESTRATEGY_H

#include <dspxproject/EditTempoTimeSignatureStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreEditTempoTimeSignatureStrategy : public DspxProject::EditTempoTimeSignatureStrategy {
    public:
        explicit CoreEditTempoTimeSignatureStrategy(DspxProject::Document *document);

        std::optional<TempoInput> requestTempo(const TempoInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        std::optional<TimeSignatureInput> requestTimeSignature(const TimeSignatureInput &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREEDITTEMPOTIMESIGNATURESTRATEGY_H
