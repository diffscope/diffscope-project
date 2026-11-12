// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELREGISTRY_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELREGISTRY_H

#include <QColor>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <coreplugin/coreglobal.h>

class QQmlComponent;

namespace Core {

    class ProjectWindowInterfacePrivate;
    class WorkspacePanelRegistryPrivate;

    // Registers panel components for a project window. All operations run on the window thread.
    class CORE_EXPORT WorkspacePanelRegistry : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(WorkspacePanelRegistry)
    public:
        struct Metadata {
            QString title;
            QString description;
            QUrl iconSource;
            QColor iconColor;
        };

        ~WorkspacePanelRegistry() override;

        // Registers a component with a DockingPane root. The provider owns the component and its creation context.
        // Returns false if the ID is empty or registered, or the component is null or uses a different engine.
        bool registerPanel(const QString &id, QQmlComponent *component, const Metadata &metadata = {});
        // Removes the registration for the ID. The window releases panel content in its event loop.
        // Returns whether the registration existed.
        bool unregisterPanel(const QString &id);

        QStringList panelIds() const;
        QQmlComponent *component(const QString &id) const;
        Metadata metadata(const QString &id) const;

    Q_SIGNALS:
        void registrationChanged(const QString &id);
        void componentStatusChanged(const QString &id);

    private:
        explicit WorkspacePanelRegistry(QObject *parent);

        friend class ProjectWindowInterfacePrivate;
        QScopedPointer<WorkspacePanelRegistryPrivate> d_ptr;
    };

}

#endif
