// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_CORECLIPBOARDEDITSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_CORECLIPBOARDEDITSTRATEGY_H

#include <dspxproject/ClipboardEditStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreClipboardEditStrategy : public DspxProject::ClipboardEditStrategy {
    public:
        explicit CoreClipboardEditStrategy(DspxProject::Document *document);

        void reportPasteFailure(QQuickWindow *window) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_CORECLIPBOARDEDITSTRATEGY_H
