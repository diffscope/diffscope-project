// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H

#include <QObject>
#include <QScopedPointer>
#include <QString>

#include <coreplugin/coreglobal.h>

namespace opendspx {
    struct Model;
}

namespace Core {

    class DspxDocument;
    class FileLocker;
    class IDspxChecker;
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

    class CORE_EXPORT ProjectDocumentContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(FileLocker *fileLocker READ fileLocker CONSTANT)
        Q_PROPERTY(DspxDocument *document READ document CONSTANT)
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
            IDspxChecker *checker = nullptr;
            bool keepFileOpen = false;
            bool acceptCheckerWarnings = false;
        };

        struct SaveOptions {
            bool compress = true;
            bool acceptIntegrityWarnings = false;
        };

        explicit ProjectDocumentContext(const Options &options, QObject *parent = nullptr);
        ~ProjectDocumentContext() override;

        FileLocker *fileLocker() const;

        DspxDocument *document() const;

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

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
