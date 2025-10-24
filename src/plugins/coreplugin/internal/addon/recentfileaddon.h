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
    public:
        explicit RecentFileAddOn(QObject *parent = nullptr);
        ~RecentFileAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QAbstractItemModel *recentFilesModel() const;

    private:
        QStandardItemModel *m_recentFilesModel;

        void updateRecentFilesModel() const;

    private Q_SLOTS:
        static void openRecentFile(int index);
        static void removeRecentFile(int index);
    };

}

#endif //DIFFSCOPE_COREPLUGIN_RECENTFILEADDON_H