#include "RecentFileAddOn.h"

#include <QDir>
#include <QFileInfo>
#include <QQmlComponent>
#include <QStandardItemModel>

#include <CoreApi/recentfilecollection.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <uishell/USDef.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    class RecentFilesModel : public QStandardItemModel {
    public:
        using QStandardItemModel::QStandardItemModel;

        QHash<int, QByteArray> roleNames() const override {
            static const QHash<int, QByteArray> m{
                {UIShell::USDef::RF_NameRole, "name"},
                {UIShell::USDef::RF_PathRole, "path"},
            };
            return m;
        }
    };

    RecentFileAddOn::RecentFileAddOn(QObject *parent) : WindowInterfaceAddOn(parent), m_recentFilesModel(new RecentFilesModel(this)), m_recoveryFilesModel(new QStandardItemModel(this)) {
        connect(CoreInterface::recentFileCollection(), &RecentFileCollection::recentFilesChanged, this, &RecentFileAddOn::updateRecentFilesModel);
        updateRecentFilesModel();
        // TODO mock data
        {
            auto item = new QStandardItem;
            item->setData("mock_recovery_file.dspx", UIShell::USDef::RF_NameRole);
            item->setData("/path/to/mock_recovery_file.dspx", UIShell::USDef::RF_PathRole);
            item->setData(QLocale().toString(QDateTime::fromString("1919-08-10T11:45:14", Qt::ISODate), QLocale::ShortFormat), UIShell::USDef::RF_LastModifiedTextRole);
            item->setData(QUrl(), UIShell::USDef::RF_ThumbnailRole);
            item->setData(QUrl("image://appicon/dspx"), UIShell::USDef::RF_IconRole);
            m_recoveryFilesModel->appendRow(item);
        }
    }

    RecentFileAddOn::~RecentFileAddOn() = default;

    void RecentFileAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "RecentFileAddOnActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "RecentFilesPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            {
                auto o = component.createWithInitialProperties({
                    {"addOn", QVariant::fromValue(this)},
                }, component.creationContext());
                o->setParent(this);
                windowInterface->actionContext()->addAction("org.diffscope.core.panel.recentFiles", o->property("recentFilesPanelComponent").value<QQmlComponent *>());
            }
            {
                auto o = component.createWithInitialProperties({
                    {"addOn", QVariant::fromValue(this)},
                    {"isRecovery", true}
                }, component.creationContext());
                o->setParent(this);
                windowInterface->actionContext()->addAction("org.diffscope.core.panel.recoveryFiles", o->property("recentFilesPanelComponent").value<QQmlComponent *>());
            }
        }
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

    QAbstractItemModel *RecentFileAddOn::recoveryFilesModel() const {
        return m_recoveryFilesModel;
    }

    bool RecentFileAddOn::isHomeWindow() const {
        return static_cast<bool>(qobject_cast<HomeWindowInterface *>(windowHandle()));
    }

    void RecentFileAddOn::updateRecentFilesModel() const {
        static const QUrl dspxIconUrl{"image://appicon/dspx"};
        static const QUrl nonExistFileIconUrl{"image://fluent-system-icons/document_error?size=48&style=regular"};
        m_recentFilesModel->clear();
        for (const auto &file : CoreInterface::recentFileCollection()->recentFiles()) {
            QFileInfo fileInfo(file);
            auto item = new QStandardItem;
            item->setData(fileInfo.baseName(), UIShell::USDef::RF_NameRole);
            item->setData(QDir::toNativeSeparators(fileInfo.absoluteFilePath()), UIShell::USDef::RF_PathRole);
            item->setData(fileInfo.exists() ? QLocale().toString(fileInfo.lastModified(), QLocale::ShortFormat) : tr("<i>File moved or deleted</i>"), UIShell::USDef::RF_LastModifiedTextRole);
            item->setData(QUrl::fromLocalFile(CoreInterface::recentFileCollection()->thumbnailPath(file)), UIShell::USDef::RF_ThumbnailRole);
            item->setData(fileInfo.exists() ? dspxIconUrl : nonExistFileIconUrl, UIShell::USDef::RF_IconRole);
            item->setData(!fileInfo.exists(), UIShell::USDef::RF_ColorizeRole);
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
