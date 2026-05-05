#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H

#include <qqmlintegration.h>

#include <QPointer>
#include <QStandardItemModel>
#include <QWindow>

#include <CoreApi/windowinterface.h>

namespace PackageManager {

    class PackageManagerAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

        Q_PROPERTY(QStandardItemModel *packageModel READ packageModel CONSTANT)
        Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)

    public:
        explicit PackageManagerAddOn(QObject *parent = nullptr);
        ~PackageManagerAddOn() override;

        QStandardItemModel *packageModel();
        bool refreshing() const;

        Q_INVOKABLE QModelIndex invalidIndex() const;
        Q_INVOKABLE void openPackageManager();
        Q_INVOKABLE void refresh();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    Q_SIGNALS:
        void refreshStarted();
        void refreshingChanged();

    private:
        bool populatePackageModel(QStandardItemModel *model, QString *errorMessage) const;
        void setRefreshing(bool refreshing);
        void showError(const QString &message) const;

        QStandardItemModel m_packageModel;
        bool m_refreshing{};
        QPointer<QWindow> m_window;
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERADDON_H
