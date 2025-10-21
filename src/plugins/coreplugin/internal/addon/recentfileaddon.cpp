#include "recentfileaddon.h"

#include <QDir>
#include <QQmlComponent>
#include <QStandardItemModel>
#include <QFileInfo>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/recentfilecollection.h>

#include <uishell/USDef.h>

#include <coreplugin/homewindowinterface.h>
#include <coreplugin/projectwindowinterface.h>
#include <coreplugin/coreinterface.h>

namespace Core::Internal {

    class RecentFilesModel : public QStandardItemModel {
    public:
        using QStandardItemModel::QStandardItemModel;

        QHash<int, QByteArray> roleNames() const override {
            static const QHash<int, QByteArray> m {
                {UIShell::USDef::RF_NameRole, "name"},
                {UIShell::USDef::RF_PathRole, "path"},
            };
            return m;
        }
    };

    RecentFileAddOn::RecentFileAddOn(QObject *parent) : WindowInterfaceAddOn(parent), m_recentFilesModel(new RecentFilesModel(this)) {
        connect(CoreInterface::recentFileCollection(), &RecentFileCollection::recentFilesChanged, this, &RecentFileAddOn::updateRecentFilesModel);
        updateRecentFilesModel();
    }

    RecentFileAddOn::~RecentFileAddOn() = default;

    void RecentFileAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "RecentFileAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        if (auto homeWindowInterface = qobject_cast<HomeWindowInterface *>(windowInterface)) {
            auto win = homeWindowInterface->window();
            win->setProperty("recentFilesModel", QVariant::fromValue(m_recentFilesModel));
            connect(win, SIGNAL(openRecentFileRequested(int)), this, SLOT(openRecentFile(int)));
            connect(win, SIGNAL(removeRecentFileRequested(int)), this, SLOT(removeRecentFile(int)));
        }
    }

    void RecentFileAddOn::extensionsInitialized() {
    }

    bool RecentFileAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QAbstractItemModel *RecentFileAddOn::recentFilesModel() const {
        return m_recentFilesModel;
    }

    void RecentFileAddOn::updateRecentFilesModel() const {
        static const QUrl dspxIconUrl{"image://appicon/dspx"};
        m_recentFilesModel->clear();
        for (const auto &file : CoreInterface::recentFileCollection()->recentFiles()) {
            QFileInfo fileInfo(file);
            auto item = new QStandardItem;
            item->setData(fileInfo.baseName(), UIShell::USDef::RF_NameRole);
            item->setData(QDir::toNativeSeparators(fileInfo.canonicalFilePath()), UIShell::USDef::RF_PathRole);
            item->setData(QLocale().toString(fileInfo.lastModified(), QLocale::ShortFormat), UIShell::USDef::RF_LastModifiedTextRole);
            item->setData(QUrl::fromLocalFile(CoreInterface::recentFileCollection()->thumbnailPath(file)), UIShell::USDef::RF_ThumbnailRole);
            item->setData(dspxIconUrl, UIShell::USDef::RF_IconRole);
            m_recentFilesModel->appendRow(item);
        }
    }

    void RecentFileAddOn::openRecentFile(int index) {
        auto filePath = CoreInterface::recentFileCollection()->recentFiles().at(index);
        CoreInterface::openFile(filePath);
    }

    void RecentFileAddOn::removeRecentFile(int index) {
        CoreInterface::recentFileCollection()->removeRecentFile(CoreInterface::recentFileCollection()->recentFiles().at(index));
    }
}