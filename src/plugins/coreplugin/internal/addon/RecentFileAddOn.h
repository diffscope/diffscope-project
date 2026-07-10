#ifndef DIFFSCOPE_COREPLUGIN_RECENTFILEADDON_H
#define DIFFSCOPE_COREPLUGIN_RECENTFILEADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

class QAbstractItemModel;
class QStandardItemModel;

namespace Core::Internal {

    class RecentFileAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAbstractItemModel *recentFilesModel READ recentFilesModel CONSTANT)
        Q_PROPERTY(QAbstractItemModel *recoveryFilesModel READ recoveryFilesModel CONSTANT)
        Q_PROPERTY(int recoveryFileCount READ recoveryFileCount NOTIFY recoveryFileCountChanged)
        Q_PROPERTY(bool isHomeWindow READ isHomeWindow CONSTANT)
    public:
        explicit RecentFileAddOn(QObject *parent = nullptr);
        ~RecentFileAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QAbstractItemModel *recentFilesModel() const;
        QAbstractItemModel *recoveryFilesModel() const;
        int recoveryFileCount() const;

        bool isHomeWindow() const;

    public Q_SLOTS:
        static void openRecentFile(int index);
        static void removeRecentFile(int index);
        Q_INVOKABLE void openRecoveryFile(int index);
        Q_INVOKABLE void removeRecoveryFile(int index);
        Q_INVOKABLE void clearRecoveryFiles();
        Q_INVOKABLE void refreshRecoveryFiles();

    Q_SIGNALS:
        void recoveryFileCountChanged();

    private:
        QStandardItemModel *m_recentFilesModel;
        QStandardItemModel *m_recoveryFilesModel;
        int m_recoveryFileCount{};

        void updateRecentFilesModel() const;
        void updateRecoveryFilesModel();

    };

}

#endif //DIFFSCOPE_COREPLUGIN_RECENTFILEADDON_H
