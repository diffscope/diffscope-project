// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREEDITSOURCESSTRATEGY_H
#define DIFFSCOPE_COREPLUGIN_COREEDITSOURCESSTRATEGY_H

#include <QCoreApplication>

#include <dspxproject/EditSourcesStrategy.h>

namespace DspxProject {
    class Document;
}

namespace Core::Internal {

    class CoreEditSourcesStrategy : public DspxProject::EditSourcesStrategy {
        Q_DECLARE_TR_FUNCTIONS(Core::Internal::CoreEditSourcesStrategy)
    public:
        explicit CoreEditSourcesStrategy(DspxProject::Document *document);

        std::optional<DspxProject::SourcesSelection> requestSources(const DspxProject::SourcesSelection &initial, QQuickWindow *window, DspxProject::DocumentEdit::Hints hints) override;
        void reportUnsupportedNoteLanguages(QQuickWindow *window) override;
        void reportOperationConflict() override;

    private:
        DspxProject::Document *const m_document;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREEDITSOURCESSTRATEGY_H
