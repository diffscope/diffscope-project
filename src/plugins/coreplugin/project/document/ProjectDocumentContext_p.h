// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H

#include <coreplugin/ProjectDocumentContext.h>

#include <memory>

#include <QByteArray>

#include <CoreApi/filelocker.h>
#include <opendspx/model.h>

class QFile;
class QLockFile;

namespace Core {

    class ProjectDocumentContextPrivate {
        Q_DECLARE_PUBLIC(ProjectDocumentContext)
    public:
        struct OpenCandidate {
            opendspx::Model model;
            std::unique_ptr<FileLocker> locker;
            ProjectFileError error;
        };

        struct PreparedSave {
            QByteArray data;
            bool hasIntegrityWarning = false;
            ProjectFileError error;
        };

        enum class OpenMode { Existing, Template, NonFileTemplate };
        enum class SaveMode { Current, As, Copy };

        // The context must be non-null.
        static ProjectDocumentContextPrivate *get(ProjectDocumentContext *context) { return context->d_func(); }

        static OpenCandidate readProjectFile(const QString &path, bool keepFileOpen);
        static OpenCandidate restoreProjectModel(const QString &directory, bool allowPartialRecovery);
        ProjectFileError loadFile(const QString &path, const QString &name, OpenMode mode, const ProjectDocumentContext::OpenOptions &options);
        ProjectFileError initialize(const opendspx::Model &model, const QString &name, std::unique_ptr<FileLocker> locker);
        static QString canonicalOrAbsolutePath(const QString &path);
        ProjectFileError validateSave(const QString &path, SaveMode mode) const;
        ProjectFileError save(const QString &path, SaveMode mode, const ProjectDocumentContext::SaveOptions &options);
        PreparedSave prepareSave(bool compress) const;
        // Current and As saves require a file locker.
        ProjectFileError writeSave(const QByteArray &data, const QString &path, SaveMode mode);

        ProjectDocumentContext *q_ptr;

        FileLocker *fileLocker{};
        DspxDocument *document{};
        ProjectDocumentContext::Options options;
        QString defaultDocumentName;
        QString recoveryDirPath;
        QFile *documentLogDevice{};
        QLockFile *documentLogLock{};
        bool writing = false;

        void writeEditorInfo(opendspx::Model &model) const;
        QString recoveryDisplayName() const;
        bool writeRecoveryNameFile() const;
        bool lockRecoveryDirectory();
        void unlockRecoveryDirectory();
        void closeDocumentLogDevice();
        bool writeRecoverySnapshot() const;
        bool openDocumentLogDevice();
        bool resetDocumentLog();
        bool setupDocumentLog();
        void cleanupDocumentLog();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
