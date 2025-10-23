#include "projectdocumentcontext.h"
#include "projectdocumentcontext_p.h"

#include <QDir>
#include <QLoggingCategory>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.core.projectdocumentcontext")

    void ProjectDocumentContextPrivate::markSaved() {
        // TODO
    }

    QByteArray ProjectDocumentContextPrivate::serializeDocument() const {
        // TODO
        return fileData_TODO;
    }

    void ProjectDocumentContextPrivate::deserializeDocument(const QByteArray &data) {
        // TODO
        fileData_TODO = data;
    }

    ProjectDocumentContext::ProjectDocumentContext(QObject *parent) : QObject(parent), d_ptr(new ProjectDocumentContextPrivate) {
        Q_D(ProjectDocumentContext);
        d->q_ptr = this;
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

    bool ProjectDocumentContext::openFile(const QString &filePath, QWindow *parent) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return false; // TODO document should not be opened
        }
        d->fileLocker = new FileLocker(this);
        if (!d->fileLocker->open(filePath)) {
            qCCritical(lcProjectDocumentContext) << "Failed to open file:" << filePath;
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to open file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(filePath), d->fileLocker->errorString()));
            return false;
        }
        if (d->fileLocker->isReadOnly()) {
            qCWarning(lcProjectDocumentContext) << "File is read-only:" << filePath;
        }
        // TODO initialize document
        bool ok;
        auto data = d->fileLocker->readData(&ok);
        if (!ok) {
            qCCritical(lcProjectDocumentContext) << "Failed to read file:" << filePath;
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to read file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(filePath), d->fileLocker->errorString()));
            return false;
        }
        d->deserializeDocument(data);
        return true;
    }

    void ProjectDocumentContext::newFile(const QDspx::Model &templateModel, bool isNonFileDocument, QWindow *parent) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return; // TODO document should not be opened
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        // TODO initialize document
        d->fileData_TODO = {"{}"};
    }

    bool ProjectDocumentContext::newFile(const QString &templateFilePath, bool isNonFileDocument, QWindow *parent) {
        Q_D(ProjectDocumentContext);
        if (false) {
            return false; // TODO document should not be opened
        }
        if (!isNonFileDocument) {
            d->fileLocker = new FileLocker(this);
        }
        // TODO initialize document
        d->fileData_TODO = {"{}"};
        return true;
    }

    bool ProjectDocumentContext::save(QWindow *parent) {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker || d->fileLocker->path().isEmpty() || d->fileLocker->isReadOnly())
            return false;
        auto data = d->serializeDocument();
        bool isSuccess = d->fileLocker->save(data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file:" << d->fileLocker->path();
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to save file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(d->fileLocker->path()), d->fileLocker->errorString()));
            return false;
        }
        d->markSaved();
        return true;
    }

    bool ProjectDocumentContext::saveAs(const QString &filePath, QWindow *parent) {
        Q_D(ProjectDocumentContext);
        if (!d->fileLocker)
            return false;
        auto data = d->serializeDocument();
        bool isSuccess = d->fileLocker->saveAs(filePath, data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save file as:" << d->fileLocker->path();
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to save file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(d->fileLocker->path()), d->fileLocker->errorString()));
            return false;
        }
        d->markSaved();
        return true;
    }

    bool ProjectDocumentContext::saveCopy(const QString &filePath, QWindow *parent) {
        Q_D(ProjectDocumentContext);
        FileLocker copyFileLocker;
        auto data = d->serializeDocument();
        bool isSuccess = copyFileLocker.saveAs(filePath, data);
        if (!isSuccess) {
            qCCritical(lcProjectDocumentContext) << "Failed to save copy file:" << d->fileLocker->path();
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to save file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(d->fileLocker->path()), d->fileLocker->errorString()));
            return false;
        }
        return true;
    }

}

#include "moc_projectdocumentcontext.cpp"