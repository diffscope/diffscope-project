// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

namespace opendspx {
    struct Model;
}

namespace Core {
    class FileLocker;
}

namespace DspxProject {

    class Document;
    class IChecker;
    class ProjectDocumentContextPrivate;

    struct ProjectFileError {
        enum Code {
            None,
            Read,
            Parse,
            Load,
            Write,
            Restore,
            Busy,
            InvalidState,
            CopyToCurrentFile,
            CheckerWarning,
            IntegrityWarning,
        };

        Code code = None;
        QString path;
        QString detail;
    };

    class DSPXPROJECT_EXPORT ProjectDocumentContext : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(Core::FileLocker *fileLocker READ fileLocker CONSTANT)
        Q_PROPERTY(Document *document READ document CONSTANT)
        Q_PROPERTY(QString defaultDocumentName READ defaultDocumentName NOTIFY defaultDocumentNameChanged)
    public:
        struct Options {
            QString editorId;
            QString editorName;
            QString editorVersion;
            QString untitledName;
            QString recoveryRootPath;
        };

        struct OpenOptions {
            IChecker *checker = nullptr;
            bool keepFileOpen = false;
            bool acceptCheckerWarnings = false;
        };

        struct SaveOptions {
            bool compress = true;
            bool acceptIntegrityWarnings = false;
        };

        explicit ProjectDocumentContext(const Options &options, QObject *parent = nullptr);
        ~ProjectDocumentContext() override;

        Core::FileLocker *fileLocker() const;

        Document *document() const;

        QString defaultDocumentName() const;
        void setDefaultDocumentName(const QString &name);

        ProjectFileError openFile(const QString &path, const OpenOptions &options);
        ProjectFileError newFile(const opendspx::Model &model, const QString &name, bool isNonFileDocument);
        ProjectFileError newFile(const QString &templatePath, const QString &name, bool isNonFileDocument, const OpenOptions &options);
        ProjectFileError save(const SaveOptions &options);
        ProjectFileError saveAs(const QString &path, const SaveOptions &options);
        ProjectFileError saveCopy(const QString &path, const SaveOptions &options);

    Q_SIGNALS:
        void saved();
        void defaultDocumentNameChanged();

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_H
