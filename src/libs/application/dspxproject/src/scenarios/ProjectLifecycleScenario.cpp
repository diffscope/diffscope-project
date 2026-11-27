// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectLifecycleScenario.h"
#include "ProjectLifecycleScenario_p.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QScopeGuard>
#include <QWindow>

#include <CoreApi/filelocker.h>

#include <opendspx/model.h>
#include <stdcorelib/support/json.h>

#include <dspxproject/Document.h>
#include <dspxproject/IChecker.h>
#include <dspxproject/ProjectLifecycleStrategy.h>
#include <dspxproject/private/ProjectDocumentContext_p.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_STATIC_LOGGING_CATEGORY(lcProjectLifecycle, "diffscope.dspxproject.projectlifecycle")

    static ProjectLifecycleScenario::Status reportFailure(ProjectLifecycleStrategy *strategy, const ProjectFileError &error, QWindow *parent) {
        qCWarning(lcProjectLifecycle) << "Project operation failed" << error.code << error.path << error.detail;
        strategy->reportError(error, parent);
        return ProjectLifecycleScenario::Status::Failed;
    }

    static ProjectLifecycleScenario::Status reportSaveResult(ProjectLifecycleStrategy *strategy, const ProjectFileError &error, QWindow *parent) {
        if (error.code == ProjectFileError::None)
            return ProjectLifecycleScenario::Status::Success;
        if (error.code == ProjectFileError::Cancelled)
            return ProjectLifecycleScenario::Status::Cancelled;
        return reportFailure(strategy, error, parent);
    }

    ProjectLifecycleScenario::ProjectLifecycleScenario(ProjectLifecycleStrategy *strategy, QObject *parent)
        : QObject(parent), d_ptr(new ProjectLifecycleScenarioPrivate{.q_ptr = this, .strategy = strategy}) {
    }

    ProjectLifecycleScenario::~ProjectLifecycleScenario() = default;

    ProjectDocumentContext *ProjectLifecycleScenario::newProject(const QString &name, QWindow *parent) {
        Q_D(ProjectLifecycleScenario);
        auto context = createUntitledProject(true, false, name);
        if (!context) {
            reportFailure(d->strategy, {ProjectFileError::Load}, parent);
            return nullptr;
        }
        const QPointer<ProjectDocumentContext> guard(context.get());
        d->strategy->presentProject(std::move(context));
        return guard.data();
    }

    std::unique_ptr<ProjectDocumentContext> ProjectLifecycleScenario::createUntitledProject(bool fileBacked, bool externallyManaged, const QString &name) {
        Q_D(ProjectLifecycleScenario);
        const auto options = d->strategy->projectLifecycleOptions();
        auto defaultClip = std::make_shared<opendspx::SingingClip>(
            options.defaultClipName.toStdString(),
            opendspx::BusControl{},
            opendspx::ClipTime{
                .pos = 0,
                .length = 0,
                .clipStart = 0,
                .clipLen = 46080
            }
        );
        defaultClip->sources = options.defaultClipSources.toOpenDSPX();
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
                        .clips = {defaultClip}
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
        auto documentOptions = options.document;
        documentOptions.externallyManaged = externallyManaged;
        documentOptions.keepFileOpenAfterSave = options.keepOpenedFileLocked;
        auto context = std::make_unique<ProjectDocumentContext>(documentOptions);
        if (context->newFile(defaultModel, documentName, !fileBacked).code != ProjectFileError::None)
            return {};
        return context;
    }

    ProjectDocumentContext *ProjectLifecycleScenario::newProjectFromModel(const opendspx::Model &model, const QString &name, QWindow *parent, bool fileBacked) {
        Q_D(ProjectLifecycleScenario);
        return d->newProjectFromModel(parent, model, name, fileBacked);
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
        if (!asTemplate) {
            if (QPointer<ProjectDocumentContext> existing = strategy->findOpenProject(fileKey)) {
                strategy->activateProject(existing.data());
                return existing.data();
            }
            if (const auto error = checkPath(fileKey, nullptr); error.code != ProjectFileError::None) {
                reportFailure(strategy, error, parentWindow);
                return nullptr;
            }
            fileOperationPaths.insert(fileKey);
        }
        const auto openingGuard = qScopeGuard([&] {
            if (!asTemplate)
                fileOperationPaths.remove(fileKey);
        });

        const auto options = strategy->projectLifecycleOptions();
        auto candidate = ProjectDocumentContextPrivate::readProjectFile(fileKey, !asTemplate && options.keepOpenedFileLocked);
        if (candidate.error.code != ProjectFileError::None) {
            reportFailure(strategy, candidate.error, parentWindow);
            return nullptr;
        }

        const auto warnings = options.checker->runCheck(candidate.model, IChecker::Strong, false);
        for (const auto &checkWarning : warnings) {
            const ProjectWarning warning{
                .code = ProjectWarning::CheckerWarning,
                .path = path,
                .message = checkWarning.message,
                .detail = checkWarning.detail,
            };
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

    ProjectDocumentContext *ProjectLifecycleScenarioPrivate::newProjectFromModel(QWindow *parent, const opendspx::Model &model, const QString &name, bool fileBacked, const QString &recoverySourceDirectory) {
        const auto options = strategy->projectLifecycleOptions();
        const auto documentName = name.isEmpty() ? options.document.untitledName + QStringLiteral(".dspx") : name;
        return presentProject(parent, model, documentName, fileBacked ? std::make_unique<Core::FileLocker>() : nullptr, options, {}, recoverySourceDirectory);
    }

    ProjectDocumentContext *ProjectLifecycleScenarioPrivate::presentProject(QWindow *parent, const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const ProjectLifecycleOptions &options, const QString &sourcePath, const QString &recoverySourceDirectory) {
        auto documentOptions = options.document;
        documentOptions.keepFileOpenAfterSave = options.keepOpenedFileLocked;
        auto context = std::make_unique<ProjectDocumentContext>(documentOptions);
        auto error = ProjectDocumentContextPrivate::get(context.get())->initialize(model, name, std::move(locker), recoverySourceDirectory);
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
        return d->newProjectFromModel(parentWindow, candidate.model, name, true, path);
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

    ProjectFileError ProjectLifecycleScenarioPrivate::checkPath(const QString &path, ProjectDocumentContext *context, bool copy) const {
        if (auto *owner = strategy->findOpenProject(path); owner && (owner != context || copy))
            return {owner == context ? ProjectFileError::CopyToCurrentFile : ProjectFileError::PathInUse, path};
        if (std::ranges::any_of(fileOperationPaths, [&](const auto &pending) { return ProjectDocumentContext::sameFilePath(pending, path); }))
            return {ProjectFileError::Busy, path};
        return {};
    }

    ProjectFileError ProjectLifecycleScenario::saveAssociatedFile(ProjectDocumentContext *context) {
        Q_D(ProjectLifecycleScenario);
        auto *locker = context->fileLocker();
        const auto path = locker ? locker->path() : QString();
        if (path.isEmpty())
            return {ProjectFileError::InvalidState, path};
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy, path};
        if (const auto error = d->checkPath(path, context); error.code != ProjectFileError::None)
            return error;
        if (locker->isFileModifiedSinceLastSave())
            return {ProjectFileError::NeedsConfirmation, path};
        const auto fileKey = ProjectDocumentContextPrivate::canonicalOrAbsolutePath(path);
        d->busyProjects.insert(context);
        d->fileOperationPaths.insert(fileKey);
        const auto guard = qScopeGuard([&] {
            d->fileOperationPaths.remove(fileKey);
            d->busyProjects.remove(context);
        });
        return context->save({.compress = d->strategy->projectLifecycleOptions().compress});
    }

    ProjectFileError ProjectLifecycleScenario::saveProjectToFile(ProjectDocumentContext *context, qsizetype maximumPathBytes,
                                                                 QWindow *parent, QObject *cancelOnDestroyed) {
        Q_D(ProjectLifecycleScenario);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy};
        d->busyProjects.insert(context);
        const auto guard = qScopeGuard([&] { d->busyProjects.remove(context); });
        return d->saveProject(context, parent, d->strategy->projectLifecycleOptions(),
                              ProjectLifecycleScenarioPrivate::SaveMode::Associate, {}, false, maximumPathBytes, cancelOnDestroyed);
    }

    ProjectFileError ProjectLifecycleScenario::restoreProjectFile(ProjectDocumentContext *context, const QString &path) {
        Q_D(ProjectLifecycleScenario);
        if (path.isEmpty())
            return {ProjectFileError::Read, path};
        const auto fileKey = ProjectDocumentContextPrivate::canonicalOrAbsolutePath(path);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy, path};
        if (const auto error = d->checkPath(fileKey, context); error.code != ProjectFileError::None)
            return error;
        d->busyProjects.insert(context);
        d->fileOperationPaths.insert(fileKey);
        const auto guard = qScopeGuard([&] {
            d->fileOperationPaths.remove(fileKey);
            d->busyProjects.remove(context);
        });

        const auto options = d->strategy->projectLifecycleOptions();
        return context->reloadFile(fileKey, {.checker = options.checker, .keepFileOpen = options.keepOpenedFileLocked});
    }

    ProjectFileError ProjectLifecycleScenario::replaceProjectFromFile(ProjectDocumentContext *context, FileReplacement replacement,
                                                                      QByteArray &snapshot, qsizetype maximumBytes,
                                                                      QWindow *parent, QObject *cancelOnDestroyed) {
        Q_D(ProjectLifecycleScenario);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy};
        d->busyProjects.insert(context);
        const auto busyGuard = qScopeGuard([&] { d->busyProjects.remove(context); });
        const QPointer<ProjectDocumentContext> project(context);
        const QPointer<QWindow> parentWindow(parent);
        const QPointer<QObject> interaction(cancelOnDestroyed);
        const auto cancelled = [&] { return !project || (cancelOnDestroyed && !interaction); };
        const auto path = d->strategy->requestPath({.purpose = ProjectPathRequest::Open, .cancelOnDestroyed = interaction}, parentWindow);
        if (path.isEmpty() || cancelled())
            return {ProjectFileError::Cancelled};

        const auto fileKey = ProjectDocumentContextPrivate::canonicalOrAbsolutePath(path);
        const bool associate = replacement == FileReplacement::Associate;
        if (associate) {
            if (fileKey.toUtf8().size() > maximumBytes)
                return {ProjectFileError::TooLarge, fileKey};
            if (const auto error = d->checkPath(fileKey, context); error.code != ProjectFileError::None)
                return error;
            d->fileOperationPaths.insert(fileKey);
        }
        const auto fileGuard = qScopeGuard([&] {
            if (associate)
                d->fileOperationPaths.remove(fileKey);
        });

        const auto options = d->strategy->projectLifecycleOptions();
        auto *contextData = ProjectDocumentContextPrivate::get(context);
        ProjectDocumentContextPrivate::OpenCandidate candidate;
        if (associate) {
            candidate = ProjectDocumentContextPrivate::readProjectFile(fileKey, options.keepOpenedFileLocked && !contextData->options.externallyManaged);
        } else {
            QFile source(fileKey);
            if (!source.open(QIODevice::ReadOnly))
                return {ProjectFileError::Read, fileKey, source.errorString()};
            const auto bytes = source.readAll();
            if (source.error() != QFileDevice::NoError)
                return {ProjectFileError::Read, fileKey, source.errorString()};
            candidate = ProjectDocumentContextPrivate::decodeProject(bytes);
            candidate.referenceFilePath = fileKey;
        }
        if (candidate.error.code != ProjectFileError::None) {
            candidate.error.path = fileKey;
            return candidate.error;
        }

        auto *controller = context->document()->transactionController();
        Core::TransactionController::ScopedReadLock readLock(controller);
        if (!readLock)
            return {ProjectFileError::Busy, fileKey};
        const auto confirm = [&](ProjectWarning::Code code, const QString &message = {}, const QString &detail = {}) {
            return d->strategy->confirmWarning({code, fileKey, message, detail, interaction}, parentWindow) && !cancelled();
        };
        for (const auto &warning : options.checker->runCheck(candidate.model, IChecker::Strong, true)) {
            if (!confirm(ProjectWarning::CheckerWarning, warning.message, warning.detail))
                return {ProjectFileError::Cancelled};
        }
        if (controller->currentStep() != controller->cleanStep() && !confirm(ProjectWarning::DirtyReplacement))
            return {ProjectFileError::Cancelled};

        readLock.release();
        ProjectDocumentContextPrivate::ReplacementSnapshot result{snapshot, maximumBytes, options.compress};
        auto error = contextData->replace(std::move(candidate), {}, associate ? nullptr : &result);
        if (error.code != ProjectFileError::None)
            error.path = fileKey;
        return error;
    }

    ProjectFileError ProjectLifecycleScenario::saveProjectSnapshot(ProjectDocumentContext *context, QByteArray &data) {
        Q_D(ProjectLifecycleScenario);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy};
        return context->saveSnapshot(data, {.compress = d->strategy->projectLifecycleOptions().compress});
    }

    ProjectFileError ProjectLifecycleScenario::detachProjectFile(ProjectDocumentContext *context, QByteArray &data, qsizetype maximumBytes) {
        Q_D(ProjectLifecycleScenario);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy};
        d->busyProjects.insert(context);
        const auto guard = qScopeGuard([&] { d->busyProjects.remove(context); });
        return ProjectDocumentContextPrivate::get(context)->detachFile(
            data, maximumBytes, {.compress = d->strategy->projectLifecycleOptions().compress});
    }

    ProjectFileError ProjectLifecycleScenario::restoreProjectSnapshot(ProjectDocumentContext *context, const QByteArray &data) {
        Q_D(ProjectLifecycleScenario);
        if (d->busyProjects.contains(context))
            return {ProjectFileError::Busy};
        d->busyProjects.insert(context);
        const auto guard = qScopeGuard([&] { d->busyProjects.remove(context); });
        return context->loadSnapshot(data, {.checker = d->strategy->projectLifecycleOptions().checker});
    }

    ProjectLifecycleScenario::Status ProjectLifecycleScenarioPrivate::saveProjectWithGuard(ProjectDocumentContext *context, QWindow *parent, SaveMode mode, const QString &path) {
        if (busyProjects.contains(context)) {
            return Status::Failed;
        }
        busyProjects.insert(context);
        const auto guard = qScopeGuard([&] {
            busyProjects.remove(context);
        });
        const QPointer<QWindow> parentWindow(parent);
        const auto error = saveProject(context, parent, strategy->projectLifecycleOptions(), mode, path);
        return reportSaveResult(strategy, error, parentWindow);
    }

    ProjectFileError ProjectLifecycleScenarioPrivate::saveProject(ProjectDocumentContext *context, QWindow *parent,
                                                                  const ProjectLifecycleOptions &options, SaveMode mode,
                                                                  QString path, bool overwriteConfirmed,
                                                                  qsizetype maximumPathBytes, QObject *cancelOnDestroyed) {
        const QPointer<QWindow> parentWindow(parent);
        const QPointer<QObject> interaction(cancelOnDestroyed);
        const bool cancellable = cancelOnDestroyed != nullptr;
        auto document = context->document();
        const auto locker = context->fileLocker();
        const auto currentPath = locker ? locker->path() : QString();
        const bool association = mode == SaveMode::Associate;
        const bool canSave = association ? currentPath.isEmpty() : mode == SaveMode::Copy || locker;
        if (!document || !canSave)
            return {ProjectFileError::InvalidState, path};

        const QPointer<ProjectDocumentContext> project(context);
        if (!document->transactionController()->canAcquireReadLock())
            return {ProjectFileError::Busy, currentPath};

        bool pathSelected = false;
        if (mode == SaveMode::As && ProjectDocumentContext::sameFilePath(currentPath, path))
            mode = SaveMode::Current;
        if (mode == SaveMode::Current) {
            path = currentPath;
            if (path.isEmpty()) {
                mode = SaveMode::As;
            } else if (options.checkExternalChanges && locker->isFileModifiedSinceLastSave()) {
                const auto choice = strategy->resolveExternalChange(currentPath, parentWindow);
                if (!project || choice == ProjectLifecycleStrategy::ExternalChangeChoice::Cancel) {
                    return {ProjectFileError::Cancelled};
                }
                if (choice == ProjectLifecycleStrategy::ExternalChangeChoice::SaveAs) {
                    mode = SaveMode::As;
                    path.clear();
                }
            }
        }
        if (path.isEmpty()) {
            const ProjectPathRequest request{mode == SaveMode::Copy ? ProjectPathRequest::SaveCopy : ProjectPathRequest::SaveAs,
                                             currentPath, context->defaultDocumentName(), cancelOnDestroyed};
            path = strategy->requestPath(request, parentWindow);
            if (!project || path.isEmpty() || (cancellable && !interaction)) {
                return {ProjectFileError::Cancelled};
            }
            pathSelected = true;
            if (mode == SaveMode::As && ProjectDocumentContext::sameFilePath(currentPath, path))
                return saveProject(context, parentWindow, options, SaveMode::Current, {});
        }
        const auto contextPrivate = ProjectDocumentContextPrivate::get(context);
        if (const auto error = contextPrivate->validateSave(path, mode); error.code != ProjectFileError::None) {
            return error;
        }
        const auto fileKey = ProjectDocumentContextPrivate::canonicalOrAbsolutePath(path);
        if (maximumPathBytes >= 0 && fileKey.toUtf8().size() > maximumPathBytes)
            return {ProjectFileError::TooLarge, path};
        if (const auto error = checkPath(fileKey, context, mode == SaveMode::Copy); error.code != ProjectFileError::None)
            return error;
        fileOperationPaths.insert(fileKey);
        const auto fileGuard = qScopeGuard([&] { fileOperationPaths.remove(fileKey); });
        if (mode != SaveMode::Current && !pathSelected && !overwriteConfirmed && QFileInfo::exists(fileKey)) {
            if (strategy->resolveExternalChange(fileKey, parentWindow) != ProjectLifecycleStrategy::ExternalChangeChoice::Overwrite || !project)
                return {ProjectFileError::Cancelled};
        }

        const QFileInfo destination(fileKey);
        const auto modifiedTime = destination.lastModified();
        const auto previousSize = destination.exists() ? destination.size() : -1;

        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock)
            return {ProjectFileError::Busy, path};
        const auto prepared = contextPrivate->prepareSave(options.compress, fileKey);
        if (prepared.error.code != ProjectFileError::None) {
            auto error = prepared.error;
            error.path = path;
            return error;
        }
        if (prepared.hasIntegrityWarning) {
            if (association)
                return {ProjectFileError::IntegrityWarning, path};
            const ProjectWarning warning{.code = ProjectWarning::SaveIntegrity, .path = path};
            if (!strategy->confirmWarning(warning, parentWindow) || !project) {
                return {ProjectFileError::Cancelled};
            }
        }

        const QFileInfo finalDestination(fileKey);
        if (finalDestination.lastModified() != modifiedTime || (finalDestination.exists() ? finalDestination.size() : -1) != previousSize) {
            return {ProjectFileError::NeedsConfirmation, path};
        }
        if (cancellable && !interaction)
            return {ProjectFileError::Cancelled};
        const auto error = contextPrivate->writeSave(prepared.data, mode == SaveMode::Current ? path : fileKey, mode);
        readLock.release();
        if (error.code != ProjectFileError::None) {
            return error;
        }
        if (project && mode != SaveMode::Copy) {
            strategy->recordRecentFile(project.data(), true);
        }
        return {};
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
            case ProjectLifecycleStrategy::CloseChoice::Save: {
                const auto error = saveProject(context, parentWindow, options, SaveMode::Current);
                return reportSaveResult(strategy, error, parentWindow);
            }
            case ProjectLifecycleStrategy::CloseChoice::Discard: {
                const auto &path = options.recoveryCopyPath;
                if (path.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath())) {
                    const ProjectFileError error{ProjectFileError::Write, path, {}};
                    return reportFailure(strategy, error, parentWindow);
                }
                const auto error = saveProject(context, parentWindow, options, SaveMode::Copy, path, true);
                return reportSaveResult(strategy, error, parentWindow);
            }
            case ProjectLifecycleStrategy::CloseChoice::Cancel:
                return Status::Cancelled;
        }
        Q_UNREACHABLE();
    }

}

#include "moc_ProjectLifecycleScenario.cpp"
