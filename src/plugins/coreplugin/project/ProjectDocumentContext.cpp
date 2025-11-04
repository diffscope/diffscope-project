#include "ProjectDocumentContext.h"
#include "ProjectDocumentContext_p.h"

#include <QDir>
#include <QLoggingCategory>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/internal/BehaviorPreference.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.core.projectdocumentcontext")

    void ProjectDocumentContextPrivate::markSaved() {
        // TODO
    }

    QByteArray ProjectDocumentContextPrivate::serializeDocument() const {
        // TODO
        QDspx::SerializationErrorList errors;
        return QDspx::Serializer::serialize(model_TODO, errors, QDspx::Serializer::CheckError);
    }

    bool ProjectDocumentContextPrivate::deserializeDocument(const QByteArray &data) {
        QDspx::SerializationErrorList errors;
        auto model = QDspx::Serializer::deserialize(data, errors);
        if (errors.containsFatal() || errors.containsError()) {
            return false;
        }
        // TODO
        model_TODO = model;
        return true;
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
        return nullptr; // TODO
    }

    OpenSaveProjectFileScenario * ProjectDocumentContext::openSaveProjectFileScenario() const {
        Q_D(const ProjectDocumentContext);
        return d->openSaveProjectFileScenario;
    }

    bool ProjectDocumentContext::openFile(const QString &filePath) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return false; // TODO document should not be opened
        }
        d->fileLocker = new FileLocker(this);
        if (!d->fileLocker->open(filePath)) {
            qCCritical(lcProjectDocumentContext) << "Failed to open file:" << filePath;
            d->openSaveProjectFileScenario->showOpenFailMessageBox(filePath, d->fileLocker->errorString());
            return false;
        }
        // TODO initialize document
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
        if (!d->deserializeDocument(data)) {
            qCCritical(lcProjectDocumentContext) << "Failed to deserialize file:" << filePath;
            d->openSaveProjectFileScenario->showDeserializationFailMessageBox(filePath);
            return false;
        }
        return true;
    }

    void ProjectDocumentContext::newFile(const QDspx::Model &templateModel, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return; // TODO document should not be opened
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        // TODO initialize document
        d->model_TODO = templateModel;
    }

    bool ProjectDocumentContext::newFile(const QString &templateFilePath, bool isNonFileDocument) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return false; // TODO document should not be opened
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
        if (!d->deserializeDocument(data)) {
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
