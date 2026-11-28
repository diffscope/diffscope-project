// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLEADDON_H
#define DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLEADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace DspxProject {
    class ProjectDocumentContext;
}

namespace Core::Internal {

    class ProjectLifecycleAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit ProjectLifecycleAddOn(QObject *parent = nullptr);

        void initialize() override;
        void extensionsInitialized() override;

        Q_INVOKABLE void newProject();
        Q_INVOKABLE void newProjectFromTemplate();
        Q_INVOKABLE void openProject();

    private:
        DspxProject::ProjectDocumentContext *projectContextForOpening() const;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTLIFECYCLEADDON_H
