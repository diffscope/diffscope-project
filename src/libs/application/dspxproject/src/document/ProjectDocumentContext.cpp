// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QUuid>

#include <opendspx/audioclip.h>
#include <opendspx/clip.h>
#include <opendspx/mixedsinger.h>
#include <opendspx/model.h>
#include <opendspx/paramcurve.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>
#include <opendspx/serializer/serializer.h>
#include <opendspx/singer.h>
#include <opendspx/singingclip.h>
#include <opendspx/singlesinger.h>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxproject/Document.h>
#include <dspxproject/IChecker.h>
#include <dspxproject/IDocumentPersistenceHook.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.dspxproject.projectdocumentcontext")

    template<typename Derived, typename Base>
    static std::shared_ptr<Derived> copyModelItem(const std::shared_ptr<Base> &item) {
        return std::make_shared<Derived>(static_cast<const Derived &>(*item));
    }

    static void detachSingers(std::vector<opendspx::SingerRef> &singers) {
        for (auto &singer : singers) {
            if (!singer) {
                continue;
            }
            switch (singer->type) {
                case opendspx::Singer::Type::Single:
                    singer = copyModelItem<opendspx::SingleSinger>(singer);
                    break;
                case opendspx::Singer::Type::Mixed: {
                    auto mixed = copyModelItem<opendspx::MixedSinger>(singer);
                    detachSingers(mixed->singers);
                    singer = std::move(mixed);
                    break;
                }
            }
        }
    }

    static void detachCurves(std::vector<opendspx::ParamCurveRef> &curves) {
        for (auto &curve : curves) {
            if (!curve) {
                continue;
            }
            switch (curve->type) {
                case opendspx::ParamCurve::Anchor:
                    curve = copyModelItem<opendspx::ParamCurveAnchor>(curve);
                    break;
                case opendspx::ParamCurve::Free:
                    curve = copyModelItem<opendspx::ParamCurveFree>(curve);
                    break;
            }
        }
    }

    static void detachSharedModelItems(opendspx::Model &model) {
        for (auto &track : model.content.tracks) {
            for (auto &clip : track.clips) {
                if (!clip) {
                    continue;
                }
                switch (clip->type) {
                    case opendspx::Clip::Type::Audio:
                        clip = copyModelItem<opendspx::AudioClip>(clip);
                        break;
                    case opendspx::Clip::Type::Singing: {
                        auto singingClip = copyModelItem<opendspx::SingingClip>(clip);
                        for (auto &[name, param] : singingClip->params) {
                            detachCurves(param.original);
                            detachCurves(param.transform);
                            detachCurves(param.edited);
                        }
                        if (singingClip->sources) {
                            detachSingers(singingClip->sources->singers);
                        }
                        clip = std::move(singingClip);
                        break;
                    }
                }
            }
        }
    }

    QString ProjectDocumentContextPrivate::recoveryDisplayName() const {
        if (document) {
            auto projectName = document->model()->projectName();
            if (!projectName.isEmpty()) {
                return projectName;
            }
        }
        if (fileLocker && !fileLocker->path().isEmpty()) {
            auto baseName = QFileInfo(fileLocker->entryName()).completeBaseName();
            if (!baseName.isEmpty()) {
                return baseName;
            }
            auto fileName = QFileInfo(fileLocker->path()).fileName();
            if (!fileName.isEmpty()) {
                return fileName;
            }
        }
        if (!defaultDocumentName.isEmpty()) {
            auto baseName = QFileInfo(defaultDocumentName).completeBaseName();
            if (!baseName.isEmpty()) {
                return baseName;
            }
            return defaultDocumentName;
        }
        return options.untitledName;
    }

    bool ProjectDocumentContextPrivate::writeRecoveryNameFile() const {
        if (!documentLogLock) {
            return false;
        }
        if (!QDir().mkpath(recoveryDirPath)) {
            qCWarning(lcProjectDocumentContext) << "Failed to create recovery directory:" << recoveryDirPath;
            return false;
        }
        QFile file(QDir(recoveryDirPath).filePath(QStringLiteral("name")));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            qCWarning(lcProjectDocumentContext) << "Failed to write recovery name file:" << file.fileName() << file.errorString();
            return false;
        }
        file.write(recoveryDisplayName().toUtf8());
        return true;
    }

    bool ProjectDocumentContextPrivate::lockRecoveryDirectory() {
        if (documentLogLock) {
            return true;
        }
        if (!QDir().mkpath(recoveryDirPath)) {
            qCWarning(lcProjectDocumentContext) << "Failed to create recovery directory:" << recoveryDirPath;
            return false;
        }
        auto *lock = new QLockFile(QDir(recoveryDirPath).filePath(QStringLiteral("active.lock")));
        if (!lock->tryLock(0)) {
            qCWarning(lcProjectDocumentContext) << "Failed to lock recovery directory:" << recoveryDirPath << lock->error();
            delete lock;
            return false;
        }
        documentLogLock = lock;
        return true;
    }

    void ProjectDocumentContextPrivate::unlockRecoveryDirectory() {
        documentLogLock->unlock();
        delete documentLogLock;
        documentLogLock = nullptr;
    }

    void ProjectDocumentContextPrivate::closeDocumentLogDevice() {
        if (document) {
            document->model()->document()->setCommitLogDevice(nullptr);
        }
        if (!documentLogDevice) {
            return;
        }
        documentLogDevice->close();
        delete documentLogDevice;
        documentLogDevice = nullptr;
    }

    bool ProjectDocumentContextPrivate::writeRecoverySnapshot() const {
        QSaveFile snapshotFile(QDir(recoveryDirPath).filePath(QStringLiteral("snapshot")));
        snapshotFile.setDirectWriteFallback(false);
        if (!snapshotFile.open(QIODevice::WriteOnly)) {
            qCWarning(lcProjectDocumentContext) << "Failed to write recovery snapshot:" << snapshotFile.fileName() << snapshotFile.errorString();
            return false;
        }
        document->model()->document()->writeSnapshot(&snapshotFile);
        if (!snapshotFile.commit()) {
            qCWarning(lcProjectDocumentContext) << "Failed to commit recovery snapshot:" << snapshotFile.fileName() << snapshotFile.errorString();
            return false;
        }
        return true;
    }

    bool ProjectDocumentContextPrivate::openDocumentLogDevice() {
        auto logFile = std::make_unique<QFile>(QDir(recoveryDirPath).filePath(QStringLiteral("logs")));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Unbuffered)) {
            qCWarning(lcProjectDocumentContext) << "Failed to open recovery log file:" << logFile->fileName() << logFile->errorString();
            return false;
        }
        document->model()->document()->setCommitLogDevice(logFile.get());
        documentLogDevice = logFile.release();
        return true;
    }

    bool ProjectDocumentContextPrivate::setupDocumentLog() {
        Q_Q(ProjectDocumentContext);
        if (!documentLogLock || !writeRecoverySnapshot() || !openDocumentLogDevice()) {
            return false;
        }
        QObject::connect(document->model(), &dspx::Model::projectNameChanged, q, [this](const QString &) {
            writeRecoveryNameFile();
        });
        writeRecoveryNameFile();
        return true;
    }

    void ProjectDocumentContextPrivate::cleanupDocumentLog() {
        closeDocumentLogDevice();
        if (documentLogLock) {
            unlockRecoveryDirectory();
            if (!preserveLog)
                QDir(recoveryDirPath).removeRecursively();
        }
    }

    ProjectDocumentContext::ProjectDocumentContext(const Options &options, QObject *parent) : QObject(parent), d_ptr(new ProjectDocumentContextPrivate) {
        Q_D(ProjectDocumentContext);
        d->q_ptr = this;
        d->options = options;
        if (!d->options.recoveryRootPath.isEmpty()) {
            const auto uuid = QUuid::createUuid().toRfc4122().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
            d->recoveryDirPath = QDir(d->options.recoveryRootPath).filePath(QString::fromLatin1(uuid));
        }
    }

    ProjectDocumentContext::~ProjectDocumentContext() {
        Q_D(ProjectDocumentContext);
        d->cleanupDocumentLog();
    }

    Core::FileLocker *ProjectDocumentContext::fileLocker() const {
        Q_D(const ProjectDocumentContext);
        return d->fileLocker;
    }

    QString ProjectDocumentContext::retainedReferenceFilePath() const {
        Q_D(const ProjectDocumentContext);
        return d->retainedReferenceFilePath;
    }

    bool ProjectDocumentContext::fileOperationsAllowed() const {
        Q_D(const ProjectDocumentContext);
        return d->fileLocker || !d->options.externallyManaged;
    }

    bool ProjectDocumentContext::sameFilePath(const QString &first, const QString &second) {
        if (first.isEmpty() || second.isEmpty())
            return false;
        if (ProjectDocumentContextPrivate::canonicalOrAbsolutePath(first) ==
            ProjectDocumentContextPrivate::canonicalOrAbsolutePath(second))
            return true;
        const QFileInfo firstInfo(first);
        const QFileInfo secondInfo(second);
        std::error_code error;
        return firstInfo.exists() && secondInfo.exists() &&
               std::filesystem::equivalent(firstInfo.filesystemFilePath(), secondInfo.filesystemFilePath(), error);
    }

    Document *ProjectDocumentContext::document() const {
        Q_D(const ProjectDocumentContext);
        return d->document;
    }

    QString ProjectDocumentContext::documentLogDirectory() const {
        Q_D(const ProjectDocumentContext);
        return d->documentLogLock ? d->recoveryDirPath : QString();
    }

    QString ProjectDocumentContext::defaultDocumentName() const {
        Q_D(const ProjectDocumentContext);
        return d->defaultDocumentName;
    }

    void ProjectDocumentContext::setDefaultDocumentName(const QString &name) {
        Q_D(ProjectDocumentContext);
        if (d->defaultDocumentName == name) {
            return;
        }
        d->defaultDocumentName = name;
        d->writeRecoveryNameFile();
        Q_EMIT defaultDocumentNameChanged();
    }

    ProjectFileError ProjectDocumentContext::openFile(const QString &path, const OpenOptions &options) {
        Q_D(ProjectDocumentContext);
        return d->loadFile(path, {}, ProjectDocumentContextPrivate::OpenMode::Existing, options);
    }

    ProjectFileError ProjectDocumentContext::reloadFile(const QString &path, const OpenOptions &options) {
        Q_D(ProjectDocumentContext);
        auto error = d->replace(d->readProjectFile(path, !d->options.externallyManaged && options.keepFileOpen), options);
        if (error.code != ProjectFileError::None)
            error.path = path;
        return error;
    }

    ProjectFileError ProjectDocumentContext::loadSnapshot(const QByteArray &data, const OpenOptions &options) {
        Q_D(ProjectDocumentContext);
        return d->replace(d->decodeProject(data), options);
    }

    ProjectFileError ProjectDocumentContext::saveSnapshot(QByteArray &data, const SaveOptions &options) {
        Q_D(ProjectDocumentContext);
        if (const auto error = d->validateSave({}, ProjectDocumentContextPrivate::SaveMode::Copy); error.code != ProjectFileError::None)
            return error;
        Core::TransactionController::ScopedReadLock readLock(d->document->transactionController());
        if (!readLock)
            return {ProjectFileError::Busy};
        auto prepared = d->prepareSave(options.compress);
        if (prepared.error.code != ProjectFileError::None)
            return prepared.error;
        if (prepared.hasIntegrityWarning && !options.acceptIntegrityWarnings)
            return {ProjectFileError::IntegrityWarning};
        data = std::move(prepared.data);
        return {};
    }

    void ProjectDocumentContext::preserveRecoveryLog() {
        Q_D(ProjectDocumentContext);
        d->preserveLog = true;
    }

    ProjectFileError ProjectDocumentContext::newFile(const opendspx::Model &model, const QString &name, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        return d->initialize(model, name, isNonFileDocument ? nullptr : std::make_unique<Core::FileLocker>());
    }

    ProjectFileError ProjectDocumentContext::newFile(const QString &templatePath, const QString &name, bool isNonFileDocument, const OpenOptions &options) {
        Q_D(ProjectDocumentContext);
        using OpenMode = ProjectDocumentContextPrivate::OpenMode;
        return d->loadFile(templatePath, name, isNonFileDocument ? OpenMode::NonFileTemplate : OpenMode::Template, options);
    }

    ProjectFileError ProjectDocumentContext::save(const SaveOptions &options) {
        Q_D(ProjectDocumentContext);
        return d->save(d->fileLocker ? d->fileLocker->path() : QString(), ProjectDocumentContextPrivate::SaveMode::Current, options);
    }

    ProjectFileError ProjectDocumentContext::saveAs(const QString &path, const SaveOptions &options) {
        Q_D(ProjectDocumentContext);
        if (d->fileLocker && sameFilePath(path, d->fileLocker->path()))
            return save(options);
        return d->save(path, ProjectDocumentContextPrivate::SaveMode::As, options);
    }

    ProjectFileError ProjectDocumentContext::saveCopy(const QString &path, const SaveOptions &options) {
        Q_D(ProjectDocumentContext);
        return d->save(path, ProjectDocumentContextPrivate::SaveMode::Copy, options);
    }

    ProjectFileError ProjectDocumentContext::saveCopy(opendspx::Model &model) {
        Q_D(ProjectDocumentContext);
        if (const auto error = d->validateSave({}, ProjectDocumentContextPrivate::SaveMode::Copy); error.code != ProjectFileError::None) {
            return error;
        }
        Core::TransactionController::ScopedReadLock readLock(d->document->transactionController());
        if (!readLock) {
            return {ProjectFileError::Busy, {}, {}};
        }
        return d->prepareSaveModel(model);
    }

    ProjectDocumentContextPrivate::OpenCandidate ProjectDocumentContextPrivate::readProjectFile(const QString &path, bool keepFileOpen) {
        auto locker = std::make_unique<Core::FileLocker>();
        if (!locker->open(path)) {
            return {.error = {ProjectFileError::Read, path, locker->errorString()}};
        }

        bool ok;
        const auto data = locker->readData(&ok);
        if (!ok) {
            return {.error = {ProjectFileError::Read, path, locker->errorString()}};
        }
        if (!keepFileOpen) {
            locker->release();
        }

        auto candidate = decodeProject(data);
        if (candidate.error.code != ProjectFileError::None)
            candidate.error.path = path;
        else
            candidate.locker = std::move(locker);
        return candidate;
    }

    ProjectDocumentContextPrivate::OpenCandidate ProjectDocumentContextPrivate::decodeProject(const QByteArray &data) {
        try {
            opendspx::SerializationErrorList errors;
            std::stringstream input(data.toStdString(), std::ios::in);
            auto model = opendspx::Serializer::deserialize(input, errors);
            if (errors.containsFatal() || errors.containsError()) {
                return {.error = {ProjectFileError::Parse}};
            }
            return {.model = std::move(model)};
        } catch (const std::exception &error) {
            return {.error = {ProjectFileError::Parse, {}, QString::fromUtf8(error.what())}};
        }
    }

    ProjectDocumentContextPrivate::OpenCandidate ProjectDocumentContextPrivate::restoreProjectModel(const QString &directory, bool allowPartialRecovery) {
        const QDir recoveryDirectory(directory);
        QFile snapshotFile(recoveryDirectory.filePath(QStringLiteral("snapshot")));
        if (!snapshotFile.open(QIODevice::ReadOnly)) {
            return {.error = {ProjectFileError::Restore, snapshotFile.fileName(), snapshotFile.errorString()}};
        }

        QFile logsFile(recoveryDirectory.filePath(QStringLiteral("logs")));
        QIODevice *logsDevice = nullptr;
        if (logsFile.exists()) {
            if (!logsFile.open(QIODevice::ReadOnly)) {
                return {.error = {ProjectFileError::Restore, logsFile.fileName(), logsFile.errorString()}};
            }
            if (logsFile.size() > 0)
                logsDevice = &logsFile;
        }

        try {
            dspx::Document::RestoreOptions options;
            if (allowPartialRecovery) {
                options |= dspx::Document::RO_DiscardInvalidCommitLogTail;
            }
            std::unique_ptr<dspx::Document> document(dspx::Document::restore(&snapshotFile, logsDevice, options));
            if (!document) {
                return {.error = {ProjectFileError::Restore, directory, QStringLiteral("Document restoration failed.")}};
            }
            return {.model = dspx::Model(document.get()).toOpenDSPX()};
        } catch (const std::exception &error) {
            return {.error = {ProjectFileError::Restore, directory, QString::fromUtf8(error.what())}};
        }
    }

    ProjectFileError ProjectDocumentContextPrivate::loadFile(const QString &path, const QString &name, OpenMode mode, const ProjectDocumentContext::OpenOptions &options) {
        if (document || initializing) {
            return {ProjectFileError::InvalidState, path, {}};
        }

        auto candidate = readProjectFile(path, mode == OpenMode::Existing && options.keepFileOpen);
        if (candidate.error.code != ProjectFileError::None) {
            return candidate.error;
        }
        if (options.checker) {
            const auto warnings = options.checker->runCheck(candidate.model, IChecker::Strong, true);
            if (!warnings.isEmpty() && !options.acceptCheckerWarnings) {
                const auto &warning = warnings.front();
                return {.code = ProjectFileError::CheckerWarning, .path = path, .detail = warning.detail, .message = warning.message};
            }
        }

        if (mode != OpenMode::Existing) {
            candidate.locker = mode == OpenMode::Template ? std::make_unique<Core::FileLocker>() : nullptr;
        }
        auto error = initialize(candidate.model, name, std::move(candidate.locker));
        if (error.code != ProjectFileError::None) {
            error.path = path;
        }
        return error;
    }

    ProjectFileError ProjectDocumentContextPrivate::initialize(const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const QString &recoverySourceDirectory) {
        Q_Q(ProjectDocumentContext);
        if (document || initializing) {
            return {ProjectFileError::InvalidState, {}, {}};
        }
        const QScopedValueRollback initializationGuard(initializing, true);
        if (!recoveryDirPath.isEmpty()) {
            lockRecoveryDirectory();
        }
        auto logGuard = qScopeGuard([this] { cleanupDocumentLog(); });
        auto newDocument = std::make_unique<Document>();
        try {
            auto projectModel = model;
            if (auto *hook = options.persistenceHook) {
                detachSharedModelItems(projectModel);
                hook->beforeLoadModel(*q, projectModel, recoverySourceDirectory);
            }
            auto storage = newDocument->model()->document();
            auto transaction = storage->engine()->beginTransaction({.undoable = false});
            storage->setTransaction(&transaction);
            const auto clearTransaction = qScopeGuard([storage] { storage->setTransaction(nullptr); });
            newDocument->loadModel(projectModel);
            transaction.commit();
        } catch (const std::exception &error) {
            return {ProjectFileError::Load, {}, QString::fromUtf8(error.what())};
        }

        defaultDocumentName = name;
        document = newDocument.release();
        document->setParent(q);
        setFileLocker(std::move(locker));
        try {
            if (setupDocumentLog()) {
                logGuard.dismiss();
            }
        } catch (const std::exception &error) {
            qCWarning(lcProjectDocumentContext) << "Failed to initialize the document log:" << error.what();
        }
        return {};
    }

    void ProjectDocumentContextPrivate::setFileLocker(std::unique_ptr<Core::FileLocker> locker) {
        Q_Q(ProjectDocumentContext);
        if (!fileLocker && !locker)
            return;
        const std::unique_ptr<Core::FileLocker> previous(fileLocker);
        fileLocker = locker.release();
        if (fileLocker) {
            fileLocker->setParent(q);
            QObject::connect(fileLocker, &Core::FileLocker::pathChanged, q, [this] { writeRecoveryNameFile(); });
        }
        Q_EMIT q->fileLockerChanged();
    }

    ProjectFileError ProjectDocumentContextPrivate::detachFile(QByteArray &data, qsizetype maximumBytes,
                                                                const ProjectDocumentContext::SaveOptions &saveOptions) {
        Q_Q(ProjectDocumentContext);
        if (!document || !fileLocker)
            return {ProjectFileError::InvalidState};
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock)
            return {ProjectFileError::Busy};
        QByteArray snapshot;
        if (const auto error = q->saveSnapshot(snapshot, saveOptions); error.code != ProjectFileError::None)
            return error;
        if (snapshot.size() > maximumBytes)
            return {ProjectFileError::TooLarge};

        const auto path = fileLocker->path();
        if (retainedReferenceFilePath.isEmpty())
            retainedReferenceFilePath = path;
        const auto name = path.isEmpty() ? defaultDocumentName : QFileInfo(path).fileName();
        q->setDefaultDocumentName(name);
        setFileLocker({});
        writeRecoveryNameFile();
        data = std::move(snapshot);
        return {};
    }

    ProjectFileError ProjectDocumentContextPrivate::replace(OpenCandidate candidate, const ProjectDocumentContext::OpenOptions &openOptions, ReplacementSnapshot *snapshot,
                                                           const QString &documentName, const QString &recoverySourceDirectory) {
        Q_Q(ProjectDocumentContext);
        if (candidate.error.code != ProjectFileError::None)
            return candidate.error;
        if (initializing || saving)
            return {ProjectFileError::Busy};
        if (openOptions.checker && !openOptions.acceptCheckerWarnings) {
            const auto warnings = openOptions.checker->runCheck(candidate.model, IChecker::Strong, true);
            if (!warnings.isEmpty())
                return {ProjectFileError::CheckerWarning, {}, warnings.front().detail, warnings.front().message};
        }
        if (!document) {
            const auto name = documentName.isEmpty() && !candidate.locker ? options.untitledName : documentName;
            return initialize(candidate.model, name, std::move(candidate.locker), recoverySourceDirectory);
        }

        const QScopedValueRollback initializingGuard(initializing, true);
        auto stagingOptions = options;
        stagingOptions.recoveryRootPath.clear();
        ProjectDocumentContext staged(stagingOptions);
        staged.d_func()->retainedReferenceFilePath = candidate.referenceFilePath;
        if (const auto error = staged.d_func()->initialize(candidate.model, documentName, {}, recoverySourceDirectory); error.code != ProjectFileError::None)
            return error;
        QByteArray snapshotData;
        if (snapshot) {
            if (const auto error = staged.saveSnapshot(snapshotData, {.compress = snapshot->compress}); error.code != ProjectFileError::None)
                return error;
            if (snapshotData.size() > snapshot->maximumBytes)
                return {ProjectFileError::TooLarge};
        }
        try {
            if (!document->transactionController()->replaceBaseline([&] {
                    if (options.persistenceHook)
                        options.persistenceHook->prepareReplacement(*q, staged);
                    const auto model = staged.document()->model()->toOpenDSPX();
                    Q_EMIT q->aboutToReload();
                    auto *storage = document->model()->document();
                    auto transaction = storage->engine()->beginTransaction({.undoable = false});
                    storage->setTransaction(&transaction);
                    const auto clearTransaction = qScopeGuard([storage] { storage->setTransaction(nullptr); });
                    const QScopedValueRollback fileAssociation(fileLocker, candidate.locker.get());
                    const QScopedValueRollback referencePath(retainedReferenceFilePath, candidate.referenceFilePath);
                    document->loadModel(model);
                    transaction.commit();
                    storage->engine()->clearUndoHistory();
                }))
                return {ProjectFileError::Busy};
        } catch (const std::exception &error) {
            return {ProjectFileError::Load, {}, QString::fromUtf8(error.what())};
        }

        retainedReferenceFilePath = std::move(candidate.referenceFilePath);
        setFileLocker(std::move(candidate.locker));
        document->selectionModel()->clearFreeParameterContext();
        document->selectionModel()->select(nullptr, dspx::SelectionModel::ClearPreviousSelection);
        if (options.persistenceHook)
            options.persistenceHook->finishReplacement(*q, staged);
        auto name = documentName;
        if (name.isEmpty() && !fileLocker)
            name = retainedReferenceFilePath.isEmpty() ? options.untitledName : QFileInfo(retainedReferenceFilePath).fileName();
        q->setDefaultDocumentName(name);
        writeRecoveryNameFile();
        if (snapshot)
            snapshot->data = std::move(snapshotData);
        Q_EMIT q->reloaded();
        return {};
    }

    QString ProjectDocumentContextPrivate::canonicalOrAbsolutePath(const QString &path) {
        const QFileInfo info(path);
        const auto canonical = info.canonicalFilePath();
        if (!canonical.isEmpty())
            return canonical;
        const auto directory = info.absoluteDir().canonicalPath();
        return directory.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : QDir(directory).filePath(info.fileName());
    }

    ProjectFileError ProjectDocumentContextPrivate::validateSave(const QString &path, SaveMode mode) const {
        const auto currentPath = fileLocker ? fileLocker->path() : QString();
        if (!document || (mode == SaveMode::Associate && (!currentPath.isEmpty() || path.isEmpty())) ||
            (!fileLocker && (mode == SaveMode::Current || mode == SaveMode::As))) {
            return {ProjectFileError::InvalidState, path, {}};
        }
        if (initializing || saving) {
            return {ProjectFileError::Busy, path, {}};
        }
        if (mode == SaveMode::Current && (path.isEmpty() || currentPath != path)) {
            return {ProjectFileError::InvalidState, path, {}};
        }
        if (mode == SaveMode::Copy && ProjectDocumentContext::sameFilePath(currentPath, path)) {
            return {ProjectFileError::CopyToCurrentFile, path, {}};
        }
        return {};
    }

    ProjectFileError ProjectDocumentContextPrivate::save(const QString &path, SaveMode mode, const ProjectDocumentContext::SaveOptions &options) {
        if (const auto error = validateSave(path, mode); error.code != ProjectFileError::None) {
            return error;
        }
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            return {ProjectFileError::Busy, path, {}};
        }
        const auto prepared = prepareSave(options.compress, path);
        if (prepared.error.code != ProjectFileError::None) {
            return {prepared.error.code, path, prepared.error.detail};
        }
        if (prepared.hasIntegrityWarning && !options.acceptIntegrityWarnings) {
            return {ProjectFileError::IntegrityWarning, path, {}};
        }
        return writeSave(prepared.data, path, mode);
    }

    ProjectFileError ProjectDocumentContextPrivate::prepareSaveModel(opendspx::Model &model, const QString &destination) {
        Q_Q(ProjectDocumentContext);
        const QScopedValueRollback saveGuard(saving, true);
        try {
            auto prepared = document->model()->toOpenDSPX();
            if (auto *hook = options.persistenceHook) {
                hook->afterSaveModel(*q, prepared, destination);
            }
            model = std::move(prepared);
            return {};
        } catch (const std::exception &error) {
            return {ProjectFileError::Write, {}, QString::fromUtf8(error.what())};
        }
    }

    ProjectDocumentContextPrivate::PreparedSave ProjectDocumentContextPrivate::prepareSave(bool compress, const QString &destination) {
        opendspx::Model model;
        if (auto error = prepareSaveModel(model, destination); error.code != ProjectFileError::None) {
            return {.error = std::move(error)};
        }
        const QScopedValueRollback saveGuard(saving, true);
        try {
            opendspx::Serializer::OutputOption outputOptions = opendspx::Serializer::WriteSchema;
            if (compress) {
                outputOptions |= opendspx::Serializer::Compress;
            }
            opendspx::SerializationErrorList errors;
            std::stringstream output(std::ios::out);
            opendspx::Serializer::serialize(output, model, errors, opendspx::Serializer::CheckError, outputOptions);
            if (errors.containsFatal() || !output) {
                return {.error = {ProjectFileError::Write, {}, {}}};
            }
            return {QByteArray::fromStdString(output.str()), errors.containsError(), {}};
        } catch (const std::exception &error) {
            return {.error = {ProjectFileError::Write, {}, QString::fromUtf8(error.what())}};
        }
    }

    ProjectFileError ProjectDocumentContextPrivate::writeSave(const QByteArray &data, const QString &path, SaveMode mode) {
        Q_Q(ProjectDocumentContext);
        if (const auto error = validateSave(path, mode); error.code != ProjectFileError::None) {
            return error;
        }
        const QScopedValueRollback saveGuard(saving, true);
        if (mode == SaveMode::Copy) {
            Core::FileLocker locker;
            if (!locker.saveAtomically(canonicalOrAbsolutePath(path), data)) {
                return {ProjectFileError::Write, path, locker.errorString()};
            }
            return {};
        }

        auto association = mode == SaveMode::Associate ? std::make_unique<Core::FileLocker>() : nullptr;
        auto *locker = association ? association.get() : fileLocker;
        const auto previousPath = mode == SaveMode::As ? locker->path() : QString();
        if (!locker->saveAtomically(canonicalOrAbsolutePath(path), data, !options.externallyManaged && options.keepFileOpenAfterSave)) {
            return {ProjectFileError::Write, path, locker->errorString()};
        }

        if (association)
            setFileLocker(std::move(association));
        if (retainedReferenceFilePath.isEmpty())
            retainedReferenceFilePath = previousPath;
        auto controller = document->transactionController();
        controller->setCleanStep(controller->currentStep());
        if (mode == SaveMode::As || mode == SaveMode::Associate) {
            q->setDefaultDocumentName({});
        }
        Q_EMIT q->saved();
        return {};
    }

}

#include "moc_ProjectDocumentContext.cpp"
