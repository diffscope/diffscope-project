// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "RecentFileAddOn.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QLocale>
#include <QQmlComponent>
#include <QStandardItemModel>

#include <CoreApi/recentfilecollection.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <uishell/USDef.h>

#include <dspxproject/ProjectLifecycleScenario.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/CoreProjectLifecycleStrategy.h>

namespace Core::Internal {

    enum RecoveryFileKind {
        RFK_LatestUnsavedProject,
        RFK_DocumentLogDirectory,
    };

    constexpr int RecoveryFileKindRole = Qt::UserRole + 100;
    constexpr int RecoveryFileStoragePathRole = Qt::UserRole + 101;

    class RecentFilesModel : public QStandardItemModel {
    public:
        using QStandardItemModel::QStandardItemModel;

        QHash<int, QByteArray> roleNames() const override {
            static const QHash<int, QByteArray> m{
                {UIShell::USDef::RF_NameRole, "name"},
                {UIShell::USDef::RF_PathRole, "path"},
                {UIShell::USDef::RF_LastModifiedTextRole, "lastModifiedText"},
                {UIShell::USDef::RF_ThumbnailRole, "thumbnail"},
                {UIShell::USDef::RF_IconRole, "icon"},
                {UIShell::USDef::RF_ColorizeRole, "colorize"},
            };
            return m;
        }
    };

    RecentFileAddOn::RecentFileAddOn(QObject *parent) : WindowInterfaceAddOn(parent), m_recentFilesModel(new RecentFilesModel(this)), m_recoveryFilesModel(new RecentFilesModel(this)) {
        connect(CoreInterface::recentFileCollection(), &RecentFileCollection::recentFilesChanged, this, &RecentFileAddOn::updateRecentFilesModel);
        updateRecentFilesModel();
        updateRecoveryFilesModel();
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
            if (!o) {
                qFatal() << component.errorString();
            }
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
                if (!o) {
                    qFatal() << component.errorString();
                }
                o->setParent(this);
                windowInterface->actionContext()->addAction("org.diffscope.core.panel.recentFiles", o->property("recentFilesPanelComponent").value<QQmlComponent *>());
            }
            {
                auto o = component.createWithInitialProperties({
                    {"addOn", QVariant::fromValue(this)},
                    {"isRecovery", true}
                }, component.creationContext());
                if (!o) {
                    qFatal() << component.errorString();
                }
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

    int RecentFileAddOn::recoveryFileCount() const {
        return m_recoveryFileCount;
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

    static bool recoveryDirectoryInUse(const QDir &directory) {
        QLockFile lock(directory.filePath(QStringLiteral("active.lock")));
        if (lock.tryLock(0)) {
            lock.unlock();
            return false;
        }
        return true;
    }

    static QString formatLastModified(const QFileInfo &fileInfo) {
        if (!fileInfo.exists()) {
            return {};
        }
        return QLocale().toString(fileInfo.lastModified(), QLocale::ShortFormat);
    }

    static QString recoveryDirectoryDisplayName(const QDir &directory) {
        QFile nameFile(directory.filePath(QStringLiteral("name")));
        if (nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            auto name = QString::fromUtf8(nameFile.readAll()).trimmed();
            if (!name.isEmpty()) {
                return name;
            }
        }
        return directory.dirName();
    }

    static QString defaultDocumentNameFromRecoveryName(const QString &name) {
        if (QFileInfo(name).suffix().compare(QStringLiteral("dspx"), Qt::CaseInsensitive) == 0) {
            return name;
        }
        return name + QStringLiteral(".dspx");
    }

    void RecentFileAddOn::updateRecoveryFilesModel() {
        static const QUrl dspxIconUrl{"image://appicon/dspx"};
        m_recoveryFilesModel->clear();
        int recoveryFileCount = 0;

        const auto latestPath = CoreProjectLifecycleStrategy::recoveryCopyPath();
        QFileInfo latestFileInfo(latestPath);
        if (latestFileInfo.exists() && latestFileInfo.isFile()) {
            auto item = new QStandardItem;
            item->setData(tr("Unsaved project from last close"), UIShell::USDef::RF_NameRole);
            item->setData(QString(), UIShell::USDef::RF_PathRole);
            item->setData(formatLastModified(latestFileInfo), UIShell::USDef::RF_LastModifiedTextRole);
            item->setData(QUrl(), UIShell::USDef::RF_ThumbnailRole);
            item->setData(dspxIconUrl, UIShell::USDef::RF_IconRole);
            item->setData(false, UIShell::USDef::RF_ColorizeRole);
            item->setData(RFK_LatestUnsavedProject, RecoveryFileKindRole);
            item->setData(QDir::toNativeSeparators(latestFileInfo.absoluteFilePath()), RecoveryFileStoragePathRole);
            m_recoveryFilesModel->appendRow(item);
        }

        QDir recoveryRoot(CoreProjectLifecycleStrategy::recoveryRootPath());
        for (const auto &directoryInfo : recoveryRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time)) {
            QDir directory(directoryInfo.absoluteFilePath());
            if (recoveryDirectoryInUse(directory)) {
                continue;
            }
            const QFileInfo logsFileInfo(directory.filePath(QStringLiteral("logs")));
            const QFileInfo snapshotFileInfo(directory.filePath(QStringLiteral("snapshot")));
            const auto modifiedText = logsFileInfo.exists()
                ? formatLastModified(logsFileInfo)
                : formatLastModified(snapshotFileInfo);

            auto item = new QStandardItem;
            item->setData(recoveryDirectoryDisplayName(directory), UIShell::USDef::RF_NameRole);
            item->setData(QString(), UIShell::USDef::RF_PathRole);
            item->setData(modifiedText, UIShell::USDef::RF_LastModifiedTextRole);
            item->setData(QUrl(), UIShell::USDef::RF_ThumbnailRole);
            item->setData(dspxIconUrl, UIShell::USDef::RF_IconRole);
            item->setData(false, UIShell::USDef::RF_ColorizeRole);
            item->setData(RFK_DocumentLogDirectory, RecoveryFileKindRole);
            item->setData(QDir::toNativeSeparators(directoryInfo.absoluteFilePath()), RecoveryFileStoragePathRole);
            m_recoveryFilesModel->appendRow(item);
            ++recoveryFileCount;
        }

        if (m_recoveryFileCount != recoveryFileCount) {
            m_recoveryFileCount = recoveryFileCount;
            Q_EMIT recoveryFileCountChanged();
        }
    }

    void RecentFileAddOn::openRecentFile(int index) const {
        auto filePath = CoreInterface::recentFileCollection()->recentFiles().at(index);
        CoreInterface::projectLifecycleScenario()->openFiles({filePath}, windowHandle()->window());
    }

    void RecentFileAddOn::removeRecentFile(int index) {
        CoreInterface::recentFileCollection()->removeRecentFile(CoreInterface::recentFileCollection()->recentFiles().at(index));
    }

    void RecentFileAddOn::openRecoveryFile(int index) {
        if (index < 0 || index >= m_recoveryFilesModel->rowCount()) {
            return;
        }
        auto item = m_recoveryFilesModel->item(index);
        const auto path = item->data(RecoveryFileStoragePathRole).toString();
        const auto kind = item->data(RecoveryFileKindRole).toInt();

        const bool isProjectCopy = kind == RFK_LatestUnsavedProject;
        const auto recoverySource = isProjectCopy ? DspxProject::ProjectRecoverySource::ProjectCopy : DspxProject::ProjectRecoverySource::DocumentLog;
        const auto name = isProjectCopy ? QString() : defaultDocumentNameFromRecoveryName(item->data(UIShell::USDef::RF_NameRole).toString());
        CoreInterface::projectLifecycleScenario()->recoverProject(recoverySource, path, name, windowHandle()->window());
    }

    void RecentFileAddOn::removeRecoveryFile(int index) {
        if (index < 0 || index >= m_recoveryFilesModel->rowCount()) {
            return;
        }
        auto item = m_recoveryFilesModel->item(index);
        const auto path = item->data(RecoveryFileStoragePathRole).toString();
        const auto kind = item->data(RecoveryFileKindRole).toInt();
        if (kind == RFK_LatestUnsavedProject) {
            QFile::remove(path);
        } else {
            QDir(path).removeRecursively();
        }
        updateRecoveryFilesModel();
    }

    void RecentFileAddOn::clearRecoveryFiles() {
        auto root = QDir(CoreProjectLifecycleStrategy::recoveryRootPath());
        QFile::remove(CoreProjectLifecycleStrategy::recoveryCopyPath());
        for (const auto &directoryInfo : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir directory(directoryInfo.absoluteFilePath());
            if (!recoveryDirectoryInUse(directory)) {
                directory.removeRecursively();
            }
        }
        updateRecoveryFilesModel();
    }

    void RecentFileAddOn::refreshRecoveryFiles() {
        updateRecoveryFilesModel();
    }
}
