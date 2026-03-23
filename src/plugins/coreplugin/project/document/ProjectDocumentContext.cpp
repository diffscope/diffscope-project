#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <sstream>

#include <application_config.h>

#include <QDir>
#include <QLoggingCategory>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>

#include <SVSCraftCore/Semver.h>
#include <SVSCraftQuick/MessageBox.h>

#include <dspxmodel/Model.h>

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
        auto model = document->model()->toOpenDspx();
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
        document->model()->fromOpenDspx(postProcessedModel);
        return true;
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
        if (!d->defaultDocumentName.isEmpty()) {
            d->defaultDocumentName.clear();
            Q_EMIT defaultDocumentNameChanged();
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
