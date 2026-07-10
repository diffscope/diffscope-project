#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <sstream>

#include <application_config.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QLoggingCategory>
#include <QUuid>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <dspxmodelCore/Document.h>
#include <opendspx/model.h>

#include <SVSCraftCore/Semver.h>
#include <SVSCraftQuick/MessageBox.h>

#include <dspxmodelORM/Model.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxCheckerRegistry.h>
#include <coreplugin/DspxDocument.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/internal/BehaviorPreference.h>

#include <opendspxserializer/serializer.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.core.projectdocumentcontext")

    static void writeEditorInfo(opendspx::Model &model) {
        model.content.global.editorId = CoreInterface::dspxEditorId();
        model.content.global.editorName = QStringLiteral("DiffScope").toStdString();
        model.content.workspace["diffscope"]["editorVersion"] = QStringLiteral(APPLICATION_SEMVER).toStdString();
    }

    static bool checkIsVersionCompatible(const QString &version) {
        if (version.isEmpty())
            return true;
        SVS::Semver currentSemver(QStringLiteral(APPLICATION_SEMVER));
        SVS::Semver fileSemver(version);
        if (fileSemver == currentSemver)
            return true;
        if (fileSemver > currentSemver)
            return false;
        if (!fileSemver.preRelease().isEmpty() || !fileSemver.build().isEmpty()) {
            return false;
        }
        return true;
    }

    void ProjectDocumentContextPrivate::markSaved() {
        Q_Q(ProjectDocumentContext);
        document->transactionController()->setCleanStep(document->transactionController()->currentStep());
        Q_EMIT q->saved();
    }

    QByteArray ProjectDocumentContextPrivate::serializeDocument(bool *hasError) const {
        opendspx::SerializationErrorList errors;
        auto model = document->model()->toOpenDSPX();
        writeEditorInfo(model);
        std::stringstream out(std::ios::out);
        opendspx::Serializer::serialize(out, model, errors, opendspx::Serializer::CheckError, Internal::BehaviorPreference::fileOption() & Internal::BehaviorPreference::FO_Compress);
        if (errors.containsError()) {
            if (hasError) {
                *hasError = true;
            }
        } else {
            if (hasError) {
                *hasError = false;
            }
        }
        return QByteArray::fromStdString(out.str());
    }

    static QString getVersionStringFromModel(const opendspx::Model &model) {
        try {
            return QString::fromStdString(model.content.workspace.at("diffscope").at("editorVersion").get<std::string>());
        } catch (const nlohmann::json::exception &) {
            return {};
        }
    }

    bool ProjectDocumentContextPrivate::initializeDocument(const opendspx::Model &model, bool doCheck) {
        Q_Q(ProjectDocumentContext);
        document = new DspxDocument(q);
        if (doCheck) {
            if (model.content.global.editorId != CoreInterface::dspxEditorId()) {
                if (!openSaveProjectFileScenario->confirmFileCreatedByAnotherApplication(QString::fromStdString(model.content.global.editorName))) {
                    return false;
                }
            } else if (auto version = getVersionStringFromModel(model); !checkIsVersionCompatible(version)) {
                if (!openSaveProjectFileScenario->confirmFileCreatedByIncompatibleVersion(version)) {
                    return false;
                }
            }
            auto customCheckResult = CoreInterface::dspxCheckerRegistry()->runCheck(model, IDspxChecker::Strong, true);
            if (!customCheckResult.isEmpty()) {
                if (!openSaveProjectFileScenario->confirmCustomCheckWarning(customCheckResult.front().message)) {
                    return false;
                }
            }
        }
        auto postProcessedModel = model;
        writeEditorInfo(postProcessedModel);
        document->loadModel(postProcessedModel);
        setupDocumentLog();
        return true;
    }

    QString ProjectDocumentContextPrivate::recoveryDisplayName() const {
        if (document && document->model()) {
            auto projectName = document->model()->projectName();
            if (!projectName.isEmpty()) {
                return projectName;
            }
        }
        if (!defaultDocumentName.isEmpty()) {
            auto baseName = QFileInfo(defaultDocumentName).completeBaseName();
            if (!baseName.isEmpty()) {
                return baseName;
            }
            return defaultDocumentName;
        }
        return CoreInterface::tr("Untitled");
    }

    bool ProjectDocumentContextPrivate::writeRecoveryNameFile() const {
        if (!documentLogEnabled || recoveryDirPath.isEmpty()) {
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
        if (!documentLogEnabled || recoveryDirPath.isEmpty()) {
            return false;
        }
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
        if (!documentLogLock) {
            return;
        }
        documentLogLock->unlock();
        delete documentLogLock;
        documentLogLock = nullptr;
    }

    void ProjectDocumentContextPrivate::closeDocumentLogDevice() {
        if (document && document->model() && document->model()->document()) {
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
        if (!documentLogEnabled || !document || !document->model() || !document->model()->document() || recoveryDirPath.isEmpty()) {
            return false;
        }
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
        if (!documentLogEnabled || !document || !document->model() || !document->model()->document() || recoveryDirPath.isEmpty()) {
            return false;
        }
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
        if (!documentLogEnabled || recoveryDirPath.isEmpty()) {
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
        if (!documentLogEnabled || !document || recoveryDirPath.isEmpty()) {
            return false;
        }
        if (!lockRecoveryDirectory()) {
            return false;
        }
        QObject::connect(document->model(), &dspx::Model::projectNameChanged, q, [this](const QString &) {
            writeRecoveryNameFile();
        });
        if (fileLocker) {
            QObject::connect(fileLocker, &FileLocker::pathChanged, q, [this] {
                writeRecoveryNameFile();
            });
        }
        writeRecoveryNameFile();
        return resetDocumentLog();
    }

    void ProjectDocumentContextPrivate::cleanupDocumentLog() {
        closeDocumentLogDevice();
        unlockRecoveryDirectory();
        if (!recoveryDirPath.isEmpty()) {
            QDir(recoveryDirPath).removeRecursively();
        }
    }

    bool ProjectDocumentContextPrivate::deserializeAndInitializeDocument(const QByteArray &data) {
        opendspx::SerializationErrorList errors;
        std::stringstream in(data.toStdString(), std::ios::in);
        auto model = opendspx::Serializer::deserialize(in, errors);
        if (errors.containsFatal() || errors.containsError()) {
            return false;
        }
        return initializeDocument(model, true);
    }

    ProjectDocumentContext::ProjectDocumentContext(QObject *parent) : QObject(parent), d_ptr(new ProjectDocumentContextPrivate) {
        Q_D(ProjectDocumentContext);
        d->q_ptr = this;
        d->openSaveProjectFileScenario = new OpenSaveProjectFileScenario(this);
        d->documentLogEnabled = Internal::BehaviorPreference::documentLogEnabled();
        auto uuidBytes = QUuid::createUuid().toRfc4122();
        auto base64 = uuidBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        d->uuidString = QString::fromLatin1(base64);
        d->recoveryDirPath = QDir(ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData))
            .filePath(QStringLiteral("recovery/%1").arg(d->uuidString));
    }

    ProjectDocumentContext::~ProjectDocumentContext() {
        Q_D(ProjectDocumentContext);
        d->cleanupDocumentLog();
    }

    FileLocker *ProjectDocumentContext::fileLocker() const {
        Q_D(const ProjectDocumentContext);
        return d->fileLocker;
    }

    DspxDocument *ProjectDocumentContext::document() const {
        Q_D(const ProjectDocumentContext);
        return d->document;
    }

    OpenSaveProjectFileScenario *ProjectDocumentContext::openSaveProjectFileScenario() const {
        Q_D(const ProjectDocumentContext);
        return d->openSaveProjectFileScenario;
    }

    QString ProjectDocumentContext::defaultDocumentName() const {
        Q_D(const ProjectDocumentContext);
        return d->defaultDocumentName;
    }

    bool ProjectDocumentContext::openFile(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        if (d->document) {
            qCWarning(lcProjectDocumentContext) << "Cannot open file: document already exists.";
            return false;
        }
        d->fileLocker = new FileLocker(this);
        if (!d->fileLocker->open(filePath)) {
            qCCritical(lcProjectDocumentContext) << "Failed to open file:" << filePath;
            d->openSaveProjectFileScenario->showOpenFailMessageBox(filePath, d->fileLocker->errorString());
            return false;
        }
        bool ok;
        auto data = d->fileLocker->readData(&ok);
        if (
#ifdef Q_OS_WIN
            !(Internal::BehaviorPreference::fileOption() & Internal::BehaviorPreference::FO_LockOpenedFiles)
#else
            true
#endif
        ) {
            d->fileLocker->release();
        }
        if (!ok) {
            qCCritical(lcProjectDocumentContext) << "Failed to read file:" << filePath;
            d->openSaveProjectFileScenario->showOpenFailMessageBox(filePath, d->fileLocker->errorString());
            return false;
        }
        if (!d->deserializeAndInitializeDocument(data)) {
            qCCritical(lcProjectDocumentContext) << "Failed to deserialize file:" << filePath;
            d->openSaveProjectFileScenario->showDeserializationFailMessageBox(filePath);
            return false;
        }
        return true;
    }

    bool ProjectDocumentContext::newFile(const opendspx::Model &templateModel, const QString &defaultDocumentName, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        if (d->document) {
            qCWarning(lcProjectDocumentContext) << "Cannot create new file: document already exists.";
            return false;
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        if (!d->initializeDocument(templateModel, false)) {
            return false;
        }
        d->defaultDocumentName = defaultDocumentName;
        Q_EMIT defaultDocumentNameChanged();
        d->writeRecoveryNameFile();
        return true;
    }

    bool ProjectDocumentContext::newFile(const QString &templateFilePath, const QString &defaultDocumentName, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        if (d->document) {
            qCWarning(lcProjectDocumentContext) << "Cannot create new file: document already exists.";
            return false;
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        FileLocker openFileLocker;
        if (!openFileLocker.open(templateFilePath)) {
            qCCritical(lcProjectDocumentContext) << "Failed to open template file:" << templateFilePath;
            d->openSaveProjectFileScenario->showOpenFailMessageBox(templateFilePath, openFileLocker.errorString());
            return false;
        }
        bool ok;
        auto data = openFileLocker.readData(&ok);
        if (!ok) {
            qCCritical(lcProjectDocumentContext) << "Failed to read template file:" << templateFilePath;
            d->openSaveProjectFileScenario->showOpenFailMessageBox(templateFilePath, d->fileLocker->errorString());
            return false;
        }
        if (!d->deserializeAndInitializeDocument(data)) {
            qCCritical(lcProjectDocumentContext) << "Failed to deserialize template file:" << templateFilePath;
            d->openSaveProjectFileScenario->showDeserializationFailMessageBox(templateFilePath);
            return false;
        }
        d->defaultDocumentName = defaultDocumentName;
        Q_EMIT defaultDocumentNameChanged();
        d->writeRecoveryNameFile();
        return true;
    }

    bool ProjectDocumentContext::save() {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker || d->fileLocker->path().isEmpty())
            return false;
        bool hasError;
        auto data = d->serializeDocument(&hasError);
        if (hasError) {
            qCWarning(lcProjectDocumentContext) << "Failed to serialize document.";
            if (!d->openSaveProjectFileScenario->confirmSaveFileIntegrity()) {
                return false;
            }
        }
        bool isSuccess = d->fileLocker->save(data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file:" << d->fileLocker->path();
            d->openSaveProjectFileScenario->showSaveFailMessageBox(d->fileLocker->path(), d->fileLocker->errorString());
            return false;
        }
        d->markSaved();
        d->resetDocumentLog();
        return true;
    }

    bool ProjectDocumentContext::saveAs(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker)
            return false;
        bool hasError;
        auto data = d->serializeDocument(&hasError);
        if (hasError) {
            qCWarning(lcProjectDocumentContext) << "Failed to serialize document.";
            if (!d->openSaveProjectFileScenario->confirmSaveFileIntegrity()) {
                return false;
            }
        }
        bool isSuccess = d->fileLocker->saveAs(filePath, data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file as:" << d->fileLocker->path();
            d->openSaveProjectFileScenario->showSaveFailMessageBox(filePath, d->fileLocker->errorString());
            return false;
        }
        d->markSaved();
        d->resetDocumentLog();
        if (!d->defaultDocumentName.isEmpty()) {
            d->defaultDocumentName.clear();
            Q_EMIT defaultDocumentNameChanged();
            d->writeRecoveryNameFile();
        }
        return true;
    }

    bool ProjectDocumentContext::saveCopy(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        FileLocker copyFileLocker;
        bool hasError;
        auto data = d->serializeDocument(&hasError);
        if (hasError) {
            qCWarning(lcProjectDocumentContext) << "Failed to serialize document.";
            if (!d->openSaveProjectFileScenario->confirmSaveFileIntegrity()) {
                return false;
            }
        }
        bool isSuccess = copyFileLocker.saveAs(filePath, data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save copy file:" << d->fileLocker->path();
            d->openSaveProjectFileScenario->showSaveFailMessageBox(filePath, copyFileLocker.errorString());
            return false;
        }
        return true;
    }

}

#include "moc_ProjectDocumentContext.cpp"
