// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREEDITLOOPSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREEDITLOOPSTRATEGY_H

#include <dspxproject/EditLoopStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreEditLoopStrategy : public DspxProject::EditLoopStrategy {
    public:
        explicit CoreEditLoopStrategy(DspxProject::Document *document);

        std::optional<Input> requestLoop(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREEDITLOOPSTRATEGY_H
