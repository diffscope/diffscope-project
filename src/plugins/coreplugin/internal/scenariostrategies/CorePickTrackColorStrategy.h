// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREPICKTRACKCOLORSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREPICKTRACKCOLORSTRATEGY_H

#include <dspxproject/PickTrackColorStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CorePickTrackColorStrategy : public DspxProject::PickTrackColorStrategy {
    public:
        explicit CorePickTrackColorStrategy(DspxProject::Document *document);

        std::optional<int> requestColor(int currentColorId, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREPICKTRACKCOLORSTRATEGY_H
