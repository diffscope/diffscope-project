#include "projectdocumentcontext.h"
#include "projectdocumentcontext_p.h"

#include <QDir>
#include <QLoggingCategory>

#include <SVSCraftQuick/MessageBox.h>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectDocumentContext, "diffscope.core.projectdocumentcontext")

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
            qCCritical(lcProjectDocumentContext) << "Failed to open file:" << lcProjectDocumentContext;
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), parent,
                tr("Failed to open file"),
                QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(filePath), d->fileLocker->errorString()));
            return false;
        }
        if (d->fileLocker->isReadOnly()) {
            qCWarning(lcProjectDocumentContext) << "File is read-only:" << lcProjectDocumentContext;
        }
        // TODO initialize document
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
        return true;
    }

}

#include "moc_projectdocumentcontext.cpp"