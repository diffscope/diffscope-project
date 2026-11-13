// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_SOURCESPROBLEMADDON_H
#define DIFFSCOPE_COREPLUGIN_SOURCESPROBLEMADDON_H

#include <memory>

#include <QPointer>

#include <CoreApi/windowinterface.h>

namespace DspxProject {
    class Document;
}

namespace dspx {
    class SingerList;
}

namespace Core::Internal {

    class SourcesProblemAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit SourcesProblemAddOn(QObject *parent = nullptr);
        ~SourcesProblemAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

    private:
        void scheduleRefresh();
        void refreshProblems();
        void watchSingerList(dspx::SingerList *singers);

        QPointer<DspxProject::Document> m_document;
        std::unique_ptr<QObject> m_modelWatcher;
        bool m_refreshPending{};
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SOURCESPROBLEMADDON_H
