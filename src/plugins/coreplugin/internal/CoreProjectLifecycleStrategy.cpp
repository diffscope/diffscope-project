// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreProjectLifecycleStrategy.h"

#include <application_config.h>

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QWindow>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/filelocker.h>
#include <CoreApi/recentfilecollection.h>
#include <CoreApi/windowsystem.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/FileOpenDispatcher.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectFileDialogs.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/ProjectStartupTimerAddOn.h>
#include <coreplugin/internal/ProjectWindowNavigatorAddOn.h>

namespace Core::Internal {

    static ProjectWindowInterface *projectWindow(const ProjectDocumentContext *context) {
        return qobject_cast<ProjectWindowInterface *>(context->parent());
    }

    CoreProjectLifecycleStrategy::CoreProjectLifecycleStrategy(IDspxChecker *checker) : m_checker(checker) {
    }

    ProjectLifecycleOptions CoreProjectLifecycleStrategy::projectLifecycleOptions() const {
        const auto fileOptions = BehaviorPreference::fileOption();
        return {
            .document = {
                .editorId = QString::fromLatin1(CoreInterface::dspxEditorId()),
                .editorName = QStringLiteral("DiffScope"),
                .editorVersion = QStringLiteral(APPLICATION_SEMVER),
                .untitledName = tr("Untitled"),
                .recoveryRootPath = BehaviorPreference::documentLogEnabled() ? recoveryRootPath() : QString(),
            },
            .defaultTrackName = tr("Unnamed track"),
            .defaultClipName = tr("Unnamed clip"),
            .recoveryCopyPath = recoveryCopyPath(),
            .checker = m_checker,
#ifdef Q_OS_WIN
            .keepOpenedFileLocked = fileOptions.testFlag(BehaviorPreference::FO_LockOpenedFiles),
#endif
            .checkExternalChanges = fileOptions.testFlag(BehaviorPreference::FO_CheckForExternalChangedOnSave),
            .compress = fileOptions.testFlag(BehaviorPreference::FO_Compress),
        };
    }

    ProjectDocumentContext *CoreProjectLifecycleStrategy::findOpenProject(const QString &canonicalPath) const {
        for (auto window : CoreInterface::windowSystem()->windows()) {
            auto project = qobject_cast<ProjectWindowInterface *>(window);
            if (!project || project->isEffectivelyClosed()) {
                continue;
            }
            auto context = project->projectDocumentContext();
            const auto locker = context->fileLocker();
            if (locker && !locker->path().isEmpty() && QFileInfo(locker->path()).canonicalFilePath() == canonicalPath) {
                return context;
            }
        }
        return nullptr;
    }

    void CoreProjectLifecycleStrategy::activateProject(ProjectDocumentContext *context) {
        ProjectWindowNavigatorAddOn::raiseWindow(projectWindow(context));
    }

    void CoreProjectLifecycleStrategy::presentProject(std::unique_ptr<ProjectDocumentContext> context) {
        ProjectStartupTimerAddOn::startTimerIfNotStarted();
        auto window = ProjectWindowInterfaceRegistry::instance()->create(context.get());
        QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
        context.release()->setParent(window);
        window->window()->setVisible(true);
        ProjectWindowNavigatorAddOn::raiseWindow(window);
        if (auto home = HomeWindowInterface::instance(); home && BehaviorPreference::startupBehavior().testFlag(BehaviorPreference::SB_CloseHomeWindowAfterOpeningProject)) {
            home->quit();
        }
    }

    void CoreProjectLifecycleStrategy::recordRecentFile(ProjectDocumentContext *context, bool captureThumbnail) {
        const auto path = context->fileLocker()->path();
        QImage thumbnail;
        if (auto project = projectWindow(context); captureThumbnail && project) {
            if (auto window = qobject_cast<QQuickWindow *>(project->window())) {
                thumbnail = window->grabWindow().scaled(400, 400, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            }
        }
        CoreInterface::recentFileCollection()->addRecentFile(path, thumbnail);
    }

    QString CoreProjectLifecycleStrategy::recoveryRootPath() {
        return QDir(ApplicationInfo::applicationLocation(ApplicationInfo::State)).filePath(QStringLiteral("recovery"));
    }

    QString CoreProjectLifecycleStrategy::recoveryCopyPath() {
        return QDir(recoveryRootPath()).filePath(QStringLiteral("latest_unsaved_project.dspx"));
    }

    void CoreProjectLifecycleStrategy::dispatchFiles(const QStringList &paths, QWindow *parent) {
        CoreInterface::fileOpenDispatcher()->openFiles(paths, parent);
    }

    QString CoreProjectLifecycleStrategy::requestPath(const ProjectPathRequest &request, QWindow *parent) {
        return ProjectFileDialogs(parent).requestPath(request);
    }

    bool CoreProjectLifecycleStrategy::confirmWarning(const ProjectWarning &warning, QWindow *parent) {
        return ProjectFileDialogs(parent).confirmWarning(warning);
    }

    ProjectLifecycleStrategy::ExternalChangeChoice CoreProjectLifecycleStrategy::resolveExternalChange(const QString &path, QWindow *parent) {
        return ProjectFileDialogs(parent).resolveExternalChange(path);
    }

    ProjectLifecycleStrategy::CloseChoice CoreProjectLifecycleStrategy::confirmClose(const QString &projectName, const QString &recoveryPath, QWindow *parent) {
        return ProjectFileDialogs(parent).confirmClose(projectName, recoveryPath);
    }

    void CoreProjectLifecycleStrategy::notifyPartialRecovery(const ProjectFileError &error, QWindow *parent) {
        ProjectFileDialogs(parent).notifyPartialRecovery(error);
    }

    void CoreProjectLifecycleStrategy::reportError(const ProjectFileError &error, QWindow *parent) {
        ProjectFileDialogs(parent).reportError(error);
    }

}
