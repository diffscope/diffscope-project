// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <exception>
#include <sstream>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QUuid>

#include <opendspx/serializer/serializer.h>
#include <stdcorelib/support/json.h>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/Model.h>
#include <dspxproject/Document.h>
#include <dspxproject/IChecker.h>
#include <transactional/TransactionController.h>

namespace DspxProject {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.dspxproject.projectdocumentcontext")

    void ProjectDocumentContextPrivate::writeEditorInfo(opendspx::Model &model) const {
        model.content.global.editorId = options.editorId.toStdString();
        model.content.global.editorName = options.editorName.toStdString();
        model.content.workspace["diffscope"]["editorVersion"] = options.editorVersion.toStdString();
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
        if (!QDir().mkpath(recoveryDirPath)) {
            qCWarning(lcProjectDocumentContext) << "Failed to create recovery directory:" << recoveryDirPath;
            return false;
        }
        QFile snapshotFile(QDir(recoveryDirPath).filePath(QStringLiteral("snapshot")));
        if (!snapshotFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(lcProjectDocumentContext) << "Failed to write recovery snapshot:" << snapshotFile.fileName() << snapshotFile.errorString();
            return false;
        }
        document->model()->document()->writeSnapshot(&snapshotFile);
        return true;
    }

    bool ProjectDocumentContextPrivate::openDocumentLogDevice() {
        auto *logFile = new QFile(QDir(recoveryDirPath).filePath(QStringLiteral("logs")));
        if (!logFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(lcProjectDocumentContext) << "Failed to open recovery log file:" << logFile->fileName() << logFile->errorString();
            delete logFile;
            return false;
        }
        documentLogDevice = logFile;
        document->model()->document()->setCommitLogDevice(documentLogDevice);
        return true;
    }

    bool ProjectDocumentContextPrivate::resetDocumentLog() {
        if (!documentLogLock) {
            return false;
        }
        closeDocumentLogDevice();
        QDir recoveryDir(recoveryDirPath);
        for (const auto &logFileName : recoveryDir.entryList({QStringLiteral("logs*")}, QDir::Files)) {
            if (!recoveryDir.remove(logFileName)) {
                qCWarning(lcProjectDocumentContext) << "Failed to remove old recovery log:" << recoveryDir.filePath(logFileName);
            }
        }
        const bool snapshotWritten = writeRecoverySnapshot();
        const bool logOpened = openDocumentLogDevice();
        return snapshotWritten && logOpened;
    }

    bool ProjectDocumentContextPrivate::setupDocumentLog() {
        Q_Q(ProjectDocumentContext);
        if (recoveryDirPath.isEmpty()) {
            return false;
        }
        if (!lockRecoveryDirectory()) {
            return false;
        }
        QObject::connect(document->model(), &dspx::Model::projectNameChanged, q, [this](const QString &) {
            writeRecoveryNameFile();
        });
        if (fileLocker) {
            QObject::connect(fileLocker, &Core::FileLocker::pathChanged, q, [this] {
                writeRecoveryNameFile();
            });
        }
        writeRecoveryNameFile();
        return resetDocumentLog();
    }

    void ProjectDocumentContextPrivate::cleanupDocumentLog() {
        closeDocumentLogDevice();
        if (documentLogLock) {
            unlockRecoveryDirectory();
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

    Document *ProjectDocumentContext::document() const {
        Q_D(const ProjectDocumentContext);
        return d->document;
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
        return d->save(path, ProjectDocumentContextPrivate::SaveMode::As, options);
    }

    ProjectFileError ProjectDocumentContext::saveCopy(const QString &path, const SaveOptions &options) {
        Q_D(ProjectDocumentContext);
        return d->save(path, ProjectDocumentContextPrivate::SaveMode::Copy, options);
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

        try {
            opendspx::SerializationErrorList errors;
            std::stringstream input(data.toStdString(), std::ios::in);
            auto model = opendspx::Serializer::deserialize(input, errors);
            if (errors.containsFatal() || errors.containsError()) {
                return {.error = {ProjectFileError::Parse, path, {}}};
            }
            return {.model = std::move(model), .locker = std::move(locker)};
        } catch (const std::exception &error) {
            return {.error = {ProjectFileError::Parse, path, QString::fromUtf8(error.what())}};
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
        if (document) {
            return {ProjectFileError::InvalidState, path, {}};
        }

        auto candidate = readProjectFile(path, mode == OpenMode::Existing && options.keepFileOpen);
        if (candidate.error.code != ProjectFileError::None) {
            return candidate.error;
        }
        if (options.checker) {
            const auto warnings = options.checker->runCheck(candidate.model, IChecker::Strong, true);
            if (!warnings.isEmpty() && !options.acceptCheckerWarnings) {
                return {ProjectFileError::CheckerWarning, path, warnings.front().message};
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

    ProjectFileError ProjectDocumentContextPrivate::initialize(const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker) {
        Q_Q(ProjectDocumentContext);
        if (document) {
            return {ProjectFileError::InvalidState, {}, {}};
        }
        auto newDocument = std::make_unique<Document>();
        try {
            auto projectModel = model;
            writeEditorInfo(projectModel);
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
        fileLocker = locker.release();
        document = newDocument.release();
        if (fileLocker) {
            fileLocker->setParent(q);
        }
        document->setParent(q);
        setupDocumentLog();
        return {};
    }

    QString ProjectDocumentContextPrivate::canonicalOrAbsolutePath(const QString &path) {
        const QFileInfo info(path);
        const auto canonical = info.canonicalFilePath();
        return canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical;
    }

    ProjectFileError ProjectDocumentContextPrivate::validateSave(const QString &path, SaveMode mode) const {
        if (!document || (!fileLocker && mode != SaveMode::Copy)) {
            return {ProjectFileError::InvalidState, path, {}};
        }
        if (writing) {
            return {ProjectFileError::Busy, path, {}};
        }
        const auto currentPath = fileLocker ? fileLocker->path() : QString();
        if (mode == SaveMode::Current && (path.isEmpty() || currentPath != path)) {
            return {ProjectFileError::InvalidState, path, {}};
        }
        if (mode == SaveMode::Copy && !currentPath.isEmpty() && canonicalOrAbsolutePath(currentPath) == canonicalOrAbsolutePath(path)) {
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
        const auto prepared = prepareSave(options.compress);
        if (prepared.error.code != ProjectFileError::None) {
            return {prepared.error.code, path, prepared.error.detail};
        }
        if (prepared.hasIntegrityWarning && !options.acceptIntegrityWarnings) {
            return {ProjectFileError::IntegrityWarning, path, {}};
        }
        return writeSave(prepared.data, path, mode);
    }

    ProjectDocumentContextPrivate::PreparedSave ProjectDocumentContextPrivate::prepareSave(bool compress) const {
        try {
            auto model = document->model()->toOpenDSPX();
            writeEditorInfo(model);
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
        const QScopedValueRollback writeGuard(writing, true);
        if (mode == SaveMode::Copy) {
            Core::FileLocker locker;
            if (!locker.saveAs(path, data)) {
                return {ProjectFileError::Write, path, locker.errorString()};
            }
            return {};
        }

        const bool written = mode == SaveMode::Current ? fileLocker->save(data) : fileLocker->saveAs(path, data);
        if (!written) {
            return {ProjectFileError::Write, path, fileLocker->errorString()};
        }

        auto controller = document->transactionController();
        controller->setCleanStep(controller->currentStep());
        resetDocumentLog();
        if (mode == SaveMode::As) {
            q->setDefaultDocumentName({});
        }
        Q_EMIT q->saved();
        return {};
    }

}

#include "moc_ProjectDocumentContext.cpp"
