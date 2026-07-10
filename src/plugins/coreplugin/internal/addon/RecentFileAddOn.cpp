#include "RecentFileAddOn.h"

#include <exception>
#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QStandardItemModel>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/recentfilecollection.h>
#include <CoreApi/runtimeinterface.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/Model.h>
#include <opendspx/model.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>

#include <uishell/USDef.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcRecentFileAddOn, "diffscope.core.recentfileaddon")

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

    static QString recoveryRootPath() {
        return QDir(ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData)).filePath(QStringLiteral("recovery"));
    }

    static QString latestUnsavedProjectPath() {
        return QDir(recoveryRootPath()).filePath(QStringLiteral("latest_unsaved_project.dspx"));
    }

    static void showRecoveryRestoreFailure() {
        SVS::MessageBox::critical(
            RuntimeInterface::qmlEngine(),
            nullptr,
            RecentFileAddOn::tr("Failed to restore recovery file"),
            RecentFileAddOn::tr("Cannot restore the recovery file.")
        );
    }

    static void showPartialRecoveryWarning() {
        SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(),
            nullptr,
            RecentFileAddOn::tr("Complete recovery failed"),
            RecentFileAddOn::tr("Complete recovery failed. Partial recovery will be attempted."),
            SVS::SVSCraft::Ok,
            SVS::SVSCraft::Ok
        );
    }

    static std::unique_ptr<dspx::Document> restoreRecoveryDocument(const QDir &recoveryDirectory,
                                                                   dspx::Document::RestoreOptions options,
                                                                   QString *errorMessage) {
        QFile snapshotFile(recoveryDirectory.filePath(QStringLiteral("snapshot")));
        if (!snapshotFile.open(QIODevice::ReadOnly)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to open recovery snapshot \"%1\": %2")
                    .arg(snapshotFile.fileName(), snapshotFile.errorString());
            }
            return nullptr;
        }

        QFile logsFile(recoveryDirectory.filePath(QStringLiteral("logs")));
        QIODevice *logsDevice = nullptr;
        if (logsFile.exists()) {
            if (!logsFile.open(QIODevice::ReadOnly)) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Failed to open recovery log \"%1\": %2")
                        .arg(logsFile.fileName(), logsFile.errorString());
                }
                return nullptr;
            }
            logsDevice = &logsFile;
        }

        try {
            auto *document = dspx::Document::restore(&snapshotFile, logsDevice, options);
            if (!document) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Document::restore returned null for recovery directory \"%1\"")
                        .arg(recoveryDirectory.absolutePath());
                }
                return nullptr;
            }
            return std::unique_ptr<dspx::Document>(document);
        } catch (const std::exception &e) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to restore document from recovery directory \"%1\": %2")
                    .arg(recoveryDirectory.absolutePath(), QString::fromUtf8(e.what()));
            }
            return nullptr;
        } catch (...) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to restore document from recovery directory \"%1\": unknown exception")
                    .arg(recoveryDirectory.absolutePath());
            }
            return nullptr;
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

        const auto latestPath = latestUnsavedProjectPath();
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

        QDir recoveryRoot(recoveryRootPath());
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

    void RecentFileAddOn::openRecentFile(int index) {
        auto filePath = CoreInterface::recentFileCollection()->recentFiles().at(index);
        CoreInterface::openFile(filePath);
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

        if (kind == RFK_LatestUnsavedProject) {
            CoreInterface::newFileFromTemplate(path);
            return;
        }

        QDir recoveryDirectory(path);
        QString completeRecoveryError;
        auto restoredDocument = restoreRecoveryDocument(recoveryDirectory, {}, &completeRecoveryError);
        if (!restoredDocument) {
            qCCritical(lcRecentFileAddOn) << "Complete recovery failed:" << completeRecoveryError;
            showPartialRecoveryWarning();

            QString partialRecoveryError;
            restoredDocument = restoreRecoveryDocument(
                recoveryDirectory,
                dspx::Document::RO_DiscardInvalidCommitLogTail,
                &partialRecoveryError
            );
            if (!restoredDocument) {
                qCCritical(lcRecentFileAddOn) << "Partial recovery failed:" << partialRecoveryError;
                showRecoveryRestoreFailure();
                return;
            }
            qCWarning(lcRecentFileAddOn) << "Partial recovery succeeded after complete recovery failed:"
                                         << recoveryDirectory.absolutePath();
        }

        opendspx::Model openDspxModel;
        try {
            dspx::Model restoredModel(restoredDocument.get());
            openDspxModel = restoredModel.toOpenDSPX();
        } catch (const std::exception &e) {
            qCCritical(lcRecentFileAddOn) << "Failed to convert restored recovery document to OpenDSPX:"
                                          << recoveryDirectory.absolutePath()
                                          << e.what();
            showRecoveryRestoreFailure();
            return;
        } catch (...) {
            qCCritical(lcRecentFileAddOn) << "Failed to convert restored recovery document to OpenDSPX:"
                                          << recoveryDirectory.absolutePath()
                                          << "Unknown exception";
            showRecoveryRestoreFailure();
            return;
        }
        auto defaultDocumentName = defaultDocumentNameFromRecoveryName(item->data(UIShell::USDef::RF_NameRole).toString());
        auto projectDocumentContext = std::make_unique<ProjectDocumentContext>();
        if (!projectDocumentContext->newFile(openDspxModel, defaultDocumentName, false)) {
            qCWarning(lcRecentFileAddOn) << "Failed to create project context from recovery directory:" << recoveryDirectory.absolutePath();
            return;
        }
        CoreInterface::createProjectWindow(projectDocumentContext.release());
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
        auto root = QDir(recoveryRootPath());
        QFile::remove(latestUnsavedProjectPath());
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
