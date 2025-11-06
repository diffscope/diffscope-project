#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <application_config.h>

#include <QDir>
#include <QLoggingCategory>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <SVSCraftCore/Semver.h>
#include <SVSCraftQuick/MessageBox.h>

#include <dspxmodel/Model.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxCheckerRegistry.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.core.projectdocumentcontext")

    static void writeEditorInfo(QDspx::Model &model) {
        model.content.global.editorId = CoreInterface::dspxEditorId();
        model.content.global.editorName = QStringLiteral("DiffScope");
        model.content.workspace["diffscope"].insert("editorVersion", QStringLiteral(APPLICATION_SEMVER));
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
        // TODO
    }

    QByteArray ProjectDocumentContextPrivate::serializeDocument() const {
        QDspx::SerializationErrorList errors;
        auto model = document->model()->toQDspx();
        writeEditorInfo(model);
        return QDspx::Serializer::serialize(model, errors, QDspx::Serializer::CheckError);
    }

    bool ProjectDocumentContextPrivate::initializeDocument(const QDspx::Model &model, bool doCheck) {
        Q_Q(ProjectDocumentContext);
        document = new DspxDocument(q);
        if (doCheck) {
            if (model.content.global.editorId != CoreInterface::dspxEditorId()) {
                if (!openSaveProjectFileScenario->confirmFileCreatedByAnotherApplication(model.content.global.editorName)) {
                    return false;
                }
            } else if (auto version = model.content.workspace.value("diffscope").value("editorVersion").toString(); !checkIsVersionCompatible(version)) {
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
        document->model()->fromQDspx(postProcessedModel);
        return true;
    }

    bool ProjectDocumentContextPrivate::deserializeAndInitializeDocument(const QByteArray &data) {
        QDspx::SerializationErrorList errors;
        auto model = QDspx::Serializer::deserialize(data, errors);
        if (errors.containsFatal() || errors.containsError()) {
            return false;
        }
        return initializeDocument(model, true);
    }

    ProjectDocumentContext::ProjectDocumentContext(QObject *parent) : QObject(parent), d_ptr(new ProjectDocumentContextPrivate) {
        Q_D(ProjectDocumentContext);
        d->q_ptr = this;
        d->openSaveProjectFileScenario = new OpenSaveProjectFileScenario(this);
    }

    ProjectDocumentContext::~ProjectDocumentContext() = default;

    FileLocker *ProjectDocumentContext::fileLocker() const {
        Q_D(const ProjectDocumentContext);
        return d->fileLocker;
    }

    DspxDocument *ProjectDocumentContext::document() const {
        Q_D(const ProjectDocumentContext);
        return d->document;
    }

    OpenSaveProjectFileScenario * ProjectDocumentContext::openSaveProjectFileScenario() const {
        Q_D(const ProjectDocumentContext);
        return d->openSaveProjectFileScenario;
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

    void ProjectDocumentContext::newFile(const QDspx::Model &templateModel, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        if (d->document) {
            qCWarning(lcProjectDocumentContext) << "Cannot create new file: document already exists.";
            return;
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        d->initializeDocument(templateModel, false);
    }

    bool ProjectDocumentContext::newFile(const QString &templateFilePath, bool isNonFileDocument) {
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
        return true;
    }

    bool ProjectDocumentContext::save() {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker || d->fileLocker->path().isEmpty())
            return false;
        auto data = d->serializeDocument();
        bool isSuccess = d->fileLocker->save(data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file:" << d->fileLocker->path();
            d->openSaveProjectFileScenario->showSaveFailMessageBox(d->fileLocker->path(), d->fileLocker->errorString());
            return false;
        }
        d->markSaved();
        return true;
    }

    bool ProjectDocumentContext::saveAs(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker)
            return false;
        auto data = d->serializeDocument();
        bool isSuccess = d->fileLocker->saveAs(filePath, data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file as:" << d->fileLocker->path();
            d->openSaveProjectFileScenario->showSaveFailMessageBox(filePath, d->fileLocker->errorString());
            return false;
        }
        d->markSaved();
        return true;
    }

    bool ProjectDocumentContext::saveCopy(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        FileLocker copyFileLocker;
        auto data = d->serializeDocument();
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
