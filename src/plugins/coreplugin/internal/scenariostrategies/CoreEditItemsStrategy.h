// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREEDITITEMSSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREEDITITEMSSTRATEGY_H

#include <dspxproject/EditItemsStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreEditItemsStrategy : public DspxProject::EditItemsStrategy {
    public:
        explicit CoreEditItemsStrategy(DspxProject::Document *document);

        bool confirmSingerLayoutChange(QQuickWindow *window) override;
        bool confirmBounceDataLoss(QQuickWindow *window) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREEDITITEMSSTRATEGY_H
