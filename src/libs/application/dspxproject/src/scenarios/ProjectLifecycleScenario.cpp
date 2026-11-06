// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectLifecycleScenario.h"
#include "ProjectLifecycleScenario_p.h"

#include <stdexcept>
#include <utility>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QScopeGuard>
#include <QWindow>

#include <CoreApi/filelocker.h>

#include <opendspx/model.h>
#include <stdcorelib/support/json.h>

#include <SVSCraftCore/Semver.h>

#include <dspxproject/Document.h>
#include <dspxproject/IChecker.h>
#include <dspxproject/ProjectLifecycleStrategy.h>
#include <dspxproject/private/ProjectDocumentContext_p.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcProjectLifecycle, "diffscope.dspxproject.projectlifecycle")

    static QString versionFromModel(const opendspx::Model *model) {
        try {
            return QString::fromStdString(model->content.workspace.at("diffscope").at("editorVersion").toString());
        } catch (const std::out_of_range &) {
            return {};
        }
    }

    static bool isVersionCompatible(const QString &version, const QString &currentVersion) {
        if (version.isEmpty()) {
            return true;
        }
        const SVS::Semver current(currentVersion);
        const SVS::Semver file(version);
        return file == current || (!(file > current) && file.preRelease().isEmpty() && file.build().isEmpty());
    }

    static ProjectLifecycleScenario::Status reportFailure(ProjectLifecycleStrategy *strategy, const ProjectFileError &error, QWindow *parent) {
        qCWarning(lcProjectLifecycle) << "Project operation failed" << error.code << error.path << error.detail;
        strategy->reportError(error, parent);
        return ProjectLifecycleScenario::Status::Failed;
    }

    ProjectLifecycleScenario::ProjectLifecycleScenario(ProjectLifecycleStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new ProjectLifecycleScenarioPrivate{.q_ptr = this, .strategy = strategy}) {
    }

    ProjectLifecycleScenario::~ProjectLifecycleScenario() = default;

    ProjectDocumentContext *ProjectLifecycleScenario::newProject(const QString &name, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        const auto options = d->strategy->projectLifecycleOptions();
        const opendspx::Model defaultModel{
            .version = opendspx::Model::Version::V1,
            .content = {
                .global = {},
                .timeline = {
                    .labels = {},
                    .tempos = {{0, 120}},
                    .timeSignatures = {{0, 4, 4}}
                },
                .tracks = {
                    opendspx::Track{
                        .name = options.defaultTrackName.toStdString(),
                        .clips = {
                            std::make_shared<opendspx::SingingClip>(
                                options.defaultClipName.toStdString(),
                                opendspx::BusControl{},
                                opendspx::ClipTime{
                                    .pos = 0,
                                    .length = 0,
                                    .clipStart = 0,
                                    .clipLen = 46080
                                }
                            )
                        }
                    }
                },
                .workspace = {
                    {"diffscope", stdc::json::Object({
                        {"keySignatures", stdc::json::Array({
                            stdc::json::Value(stdc::json::Object({
                                {"pos", 0},
                                {"mode", 2741},
                                {"tonality", 0},
                                {"accidentalType", 1}
                            }))
                        })}
                    })}
                }
            }
        };
        const auto documentName = name.isEmpty() ? options.document.untitledName + QStringLiteral(".dspx") : name;
        return d->presentProject(parent, defaultModel, documentName, std::make_unique<Core::FileLocker>(), options);
    }

    ProjectDocumentContext *ProjectLifecycleScenario::newProjectFromModel(const opendspx::Model &model, const QString &name, QWindow *parent, bool fileBacked) {
        Q_D(ProjectLifecycleScenario);
        const auto options = d->strategy->projectLifecycleOptions();
        const auto documentName = name.isEmpty() ? options.document.untitledName + QStringLiteral(".dspx") : name;
        return d->presentProject(parent, model, documentName, fileBacked ? std::make_unique<Core::FileLocker>() : nullptr, options);
    }

    ProjectDocumentContext *ProjectLifecycleScenario::newProjectFromTemplate(const QString &path, QWindow *parent, const QString &name) {
        Q_D(ProjectLifecycleScenario);
        return d->openProject(parent, path, true, name);
    }

    ProjectDocumentContext *ProjectLifecycleScenario::openProject(const QString &path, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        return d->openProject(parent, path, false, {});
    }

    void ProjectLifecycleScenario::openFiles(const QStringList &paths, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        if (!paths.isEmpty()) {
            d->strategy->dispatchFiles(paths, parent);
            return;
        }
        const QPointer<QWindow> parentWindow(parent);
        const ProjectPathRequest request{ProjectPathRequest::Open, {}, {}};
        const auto path = d->strategy->requestPath(request, parentWindow);
        if (!path.isEmpty()) {
            d->strategy->dispatchFiles({path}, parentWindow);
        }
    }

    ProjectDocumentContext *ProjectLifecycleScenarioPrivate::openProject(QWindow *parent, QString path, bool asTemplate, const QString &name) {
        const QPointer<QWindow> parentWindow(parent);
        if (path.isEmpty()) {
            const ProjectPathRequest request{asTemplate ? ProjectPathRequest::Template : ProjectPathRequest::Open, {}, {}};
            path = strategy->requestPath(request, parentWindow);
            if (path.isEmpty()) {
                return nullptr;
            }
        }

        const auto fileKey = ProjectDocumentContextPrivate::canonicalOrAbsolutePath(path);
        if (openingPaths.contains(fileKey)) {
            return nullptr;
        }
        openingPaths.insert(fileKey);
        const auto openingGuard = qScopeGuard([&] {
            openingPaths.remove(fileKey);
        });

        if (!asTemplate) {
            if (QPointer<ProjectDocumentContext> existing = strategy->findOpenProject(fileKey)) {
                strategy->activateProject(existing.data());
                return existing.data();
            }
        }

        const auto options = strategy->projectLifecycleOptions();
        auto candidate = ProjectDocumentContextPrivate::readProjectFile(path, !asTemplate && options.keepOpenedFileLocked);
        if (candidate.error.code != ProjectFileError::None) {
            reportFailure(strategy, candidate.error, parentWindow);
            return nullptr;
        }

        const auto &global = candidate.model.content.global;
        if (QString::fromStdString(global.editorId) != options.document.editorId) {
            const ProjectWarning warning{ProjectWarning::OtherEditor, path, QString::fromStdString(global.editorName)};
            if (!strategy->confirmWarning(warning, parentWindow)) {
                return nullptr;
            }
        } else if (const auto version = versionFromModel(&candidate.model); !isVersionCompatible(version, options.document.editorVersion)) {
            const ProjectWarning warning{ProjectWarning::IncompatibleVersion, path, version};
            if (!strategy->confirmWarning(warning, parentWindow)) {
                return nullptr;
            }
        }
        const auto warnings = options.checker->runCheck(candidate.model, IChecker::Strong, true);
        if (!warnings.isEmpty()) {
            const ProjectWarning warning{ProjectWarning::CheckerWarning, path, warnings.front().message};
            if (!strategy->confirmWarning(warning, parentWindow)) {
                return nullptr;
            }
        }

        if (asTemplate) {
            const auto documentName = name.isEmpty() ? options.document.untitledName + QStringLiteral(".dspx") : name;
            return presentProject(parentWindow, candidate.model, documentName, std::make_unique<Core::FileLocker>(), options, path);
        }
        const QPointer<ProjectDocumentContext> context(presentProject(parentWindow, candidate.model, {}, std::move(candidate.locker), options, path));
        if (context) {
            strategy->recordRecentFile(context.data(), false);
        }
        return context.data();
    }

    ProjectDocumentContext *ProjectLifecycleScenarioPrivate::presentProject(QWindow *parent, const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const ProjectLifecycleOptions &options, const QString &sourcePath) {
        auto context = std::make_unique<ProjectDocumentContext>(options.document);
        auto error = ProjectDocumentContextPrivate::get(context.get())->initialize(model, name, std::move(locker));
        if (error.code != ProjectFileError::None) {
            error.path = sourcePath;
            reportFailure(strategy, error, parent);
            return nullptr;
        }
        const QPointer<ProjectDocumentContext> guard(context.get());
        strategy->presentProject(std::move(context));
        return guard.data();
    }

    ProjectDocumentContext *ProjectLifecycleScenario::recoverProject(ProjectRecoverySource source, const QString &path, const QString &name, QWindow *parent) {
        if (path.isEmpty()) {
            return nullptr;
        }
        if (source == ProjectRecoverySource::ProjectCopy) {
            return newProjectFromTemplate(path, parent, name);
        }

        Q_D(ProjectLifecycleScenario);
        const QPointer<QWindow> parentWindow(parent);
        auto candidate = ProjectDocumentContextPrivate::restoreProjectModel(path, false);
        if (candidate.error.code != ProjectFileError::None) {
            qCWarning(lcProjectLifecycle) << "Complete project recovery failed" << candidate.error.path << candidate.error.detail;
            d->strategy->notifyPartialRecovery(candidate.error, parentWindow);
            candidate = ProjectDocumentContextPrivate::restoreProjectModel(path, true);
            if (candidate.error.code != ProjectFileError::None) {
                reportFailure(d->strategy, candidate.error, parentWindow);
                return nullptr;
            }
        }
        return newProjectFromModel(candidate.model, name, parentWindow);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenario::saveProject(ProjectDocumentContext *context, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        return d->saveProjectWithGuard(context, parent, ProjectLifecycleScenarioPrivate::SaveMode::Current);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenario::saveProjectAs(ProjectDocumentContext *context, const QString &path, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        return d->saveProjectWithGuard(context, parent, ProjectLifecycleScenarioPrivate::SaveMode::As, path);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenario::saveProjectCopy(ProjectDocumentContext *context, const QString &path, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        return d->saveProjectWithGuard(context, parent, ProjectLifecycleScenarioPrivate::SaveMode::Copy, path);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenarioPrivate::saveProjectWithGuard(ProjectDocumentContext *context, QWindow *parent, SaveMode mode, const QString &path) {
        if (busyProjects.contains(context)) {
            return Status::Failed;
        }
        busyProjects.insert(context);
        const auto guard = qScopeGuard([&] {
            busyProjects.remove(context);
        });
        const auto options = strategy->projectLifecycleOptions();
        return saveProject(context, parent, options, mode, path);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenarioPrivate::saveProject(ProjectDocumentContext *context, QWindow *parent, const ProjectLifecycleOptions &options, SaveMode mode, QString path) {
        const QPointer<QWindow> parentWindow(parent);
        auto document = context->document();
        const auto locker = context->fileLocker();
        if (!locker && mode != SaveMode::Copy) {
            const ProjectFileError error{ProjectFileError::InvalidState, path, {}};
            return reportFailure(strategy, error, parentWindow);
        }

        const QPointer<ProjectDocumentContext> project(context);
        const auto currentPath = locker ? locker->path() : QString();
        if (!document->transactionController()->canAcquireReadLock()) {
            const ProjectFileError error{ProjectFileError::Busy, currentPath, {}};
            return reportFailure(strategy, error, parentWindow);
        }

        if (mode == SaveMode::Current) {
            path = currentPath;
            if (path.isEmpty()) {
                mode = SaveMode::As;
            } else if (options.checkExternalChanges && locker->isFileModifiedSinceLastSave()) {
                const auto choice = strategy->resolveExternalChange(currentPath, parentWindow);
                if (!project || choice == ProjectLifecycleStrategy::ExternalChangeChoice::Cancel) {
                    return Status::Cancelled;
                }
                if (choice == ProjectLifecycleStrategy::ExternalChangeChoice::SaveAs) {
                    mode = SaveMode::As;
                    path.clear();
                }
            }
        }
        if (path.isEmpty()) {
            const ProjectPathRequest request{mode == SaveMode::Copy ? ProjectPathRequest::SaveCopy : ProjectPathRequest::SaveAs, currentPath, context->defaultDocumentName()};
            path = strategy->requestPath(request, parentWindow);
            if (!project || path.isEmpty()) {
                return Status::Cancelled;
            }
        }
        const auto contextPrivate = ProjectDocumentContextPrivate::get(context);
        if (const auto error = contextPrivate->validateSave(path, mode); error.code != ProjectFileError::None) {
            return reportFailure(strategy, error, parentWindow);
        }

        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            const ProjectFileError error{ProjectFileError::Busy, path, {}};
            return reportFailure(strategy, error, parentWindow);
        }
        const auto prepared = contextPrivate->prepareSave(options.compress);
        if (prepared.error.code != ProjectFileError::None) {
            auto error = prepared.error;
            error.path = path;
            readLock.release();
            return reportFailure(strategy, error, parentWindow);
        }
        if (prepared.hasIntegrityWarning) {
            const ProjectWarning warning{ProjectWarning::SaveIntegrity, path, {}};
            if (!strategy->confirmWarning(warning, parentWindow) || !project) {
                return Status::Cancelled;
            }
        }

        const auto error = contextPrivate->writeSave(prepared.data, path, mode);
        readLock.release();
        if (error.code != ProjectFileError::None) {
            return reportFailure(strategy, error, parentWindow);
        }
        if (project && mode != SaveMode::Copy) {
            strategy->recordRecentFile(project.data(), true);
        }
        return Status::Success;
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenario::prepareClose(ProjectDocumentContext *context, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        return d->prepareClose(context, parent);
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenarioPrivate::prepareClose(ProjectDocumentContext *context, QWindow *parent) {
        const QPointer<QWindow> parentWindow(parent);
        if (busyProjects.contains(context)) {
            return Status::Failed;
        }
        const auto locker = context->fileLocker();
        if (!locker) {
            return Status::Success;
        }
        busyProjects.insert(context);
        const auto guard = qScopeGuard([&] {
            busyProjects.remove(context);
        });

        const QPointer<ProjectDocumentContext> contextGuard(context);
        const auto controller = context->document()->transactionController();
        const auto currentPath = locker->path();
        Core::TransactionController::ScopedReadLock readLock(controller);
        if (!readLock) {
            const ProjectFileError error{ProjectFileError::Busy, currentPath, {}};
            return reportFailure(strategy, error, parentWindow);
        }
        if (controller->cleanStep() == controller->currentStep() && !locker->isFileModifiedSinceLastSave()) {
            return Status::Success;
        }

        const auto options = strategy->projectLifecycleOptions();
        const auto name = currentPath.isEmpty() ? context->defaultDocumentName() : locker->entryName();
        const auto choice = strategy->confirmClose(name, options.recoveryCopyPath, parentWindow);
        if (!contextGuard) {
            return Status::Cancelled;
        }
        switch (choice) {
            case ProjectLifecycleStrategy::CloseChoice::Save:
                return saveProject(context, parentWindow, options, SaveMode::Current);
            case ProjectLifecycleStrategy::CloseChoice::Discard: {
                const auto &path = options.recoveryCopyPath;
                if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
                    const ProjectFileError error{ProjectFileError::Write, path, {}};
                    return reportFailure(strategy, error, parentWindow);
                }
                return saveProject(context, parentWindow, options, SaveMode::Copy, path);
            }
            case ProjectLifecycleStrategy::CloseChoice::Cancel:
                return Status::Cancelled;
        }
        Q_UNREACHABLE();
    }

}

#include "moc_ProjectLifecycleScenario.cpp"
