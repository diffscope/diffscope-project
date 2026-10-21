// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H

#include <qqmlintegration.h>

#include <QList>
#include <QPointer>
#include <QStandardItemModel>
#include <QUrl>
#include <QWindow>

#include <CoreApi/windowinterface.h>

namespace PackageManager {

    class PackageManagerAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

        Q_PROPERTY(QStandardItemModel *packageModel READ packageModel CONSTANT)
        Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)
        Q_PROPERTY(bool uninstalling READ uninstalling NOTIFY uninstallingChanged)
        Q_PROPERTY(bool installing READ installing NOTIFY installingChanged)

    public:
        explicit PackageManagerAddOn(QObject *parent = nullptr);
        ~PackageManagerAddOn() override;

        QStandardItemModel *packageModel();
        bool refreshing() const;
        bool uninstalling() const;
        bool installing() const;

        Q_INVOKABLE QModelIndex invalidIndex() const;
        Q_INVOKABLE void openPackageManager();
        Q_INVOKABLE void refresh();
        Q_INVOKABLE void uninstallPackage(int index);
        Q_INVOKABLE void installPackages(const QList<QUrl> &files);

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    Q_SIGNALS:
        void refreshStarted();
        void refreshingChanged();
        void uninstallingChanged();
        void installingChanged();

    private:
        bool populatePackageModel(QStandardItemModel *model, QString *errorMessage) const;
        bool reloadPackageModel(QString *errorMessage, bool clearModel = false);
        void setRefreshing(bool refreshing);
        void setUninstalling(bool uninstalling);
        void setInstalling(bool installing);
        void showError(const QString &message) const;

        QStandardItemModel m_packageModel;
        bool m_refreshing{};
        bool m_uninstalling{};
        bool m_installing{};
        QPointer<QWindow> m_window;
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H
