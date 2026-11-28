// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_P_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_P_H

#include <dspxproject/ProjectDocumentContext.h>

#include <memory>

#include <QByteArray>

#include <CoreApi/filelocker.h>
#include <opendspx/model.h>

class QFile;
class QLockFile;

namespace DspxProject {

    class ProjectDocumentContextPrivate {
        Q_DECLARE_PUBLIC(ProjectDocumentContext)
    public:
        struct OpenCandidate {
            opendspx::Model model;
            std::unique_ptr<Core::FileLocker> locker;
            ProjectFileError error;
            QString referenceFilePath;
        };

        struct ReplacementSnapshot {
            QByteArray &data;
            qsizetype maximumBytes;
            bool compress;
        };

        struct PreparedSave {
            QByteArray data;
            bool hasIntegrityWarning = false;
            ProjectFileError error;
        };

        enum class OpenMode { Existing, Template, NonFileTemplate };
        enum class SaveMode { Current, As, Copy, Associate };

        static ProjectDocumentContextPrivate *get(ProjectDocumentContext *context) { return context->d_func(); }

        static OpenCandidate readProjectFile(const QString &path, bool keepFileOpen);
        static OpenCandidate decodeProject(const QByteArray &data);
        static OpenCandidate restoreProjectModel(const QString &directory, bool allowPartialRecovery);
        ProjectFileError loadFile(const QString &path, const QString &name, OpenMode mode, const ProjectDocumentContext::OpenOptions &options);
        ProjectFileError initialize(const opendspx::Model &model, const QString &name, std::unique_ptr<Core::FileLocker> locker, const QString &recoverySourceDirectory = {});
        ProjectFileError replace(OpenCandidate candidate, const ProjectDocumentContext::OpenOptions &options, ReplacementSnapshot *snapshot = nullptr,
                                 const QString &documentName = {}, const QString &recoverySourceDirectory = {});
        void setFileLocker(std::unique_ptr<Core::FileLocker> locker);
        ProjectFileError detachFile(QByteArray &data, qsizetype maximumBytes, const ProjectDocumentContext::SaveOptions &options);
        static QString canonicalOrAbsolutePath(const QString &path);
        ProjectFileError validateSave(const QString &path, SaveMode mode) const;
        ProjectFileError save(const QString &path, SaveMode mode, const ProjectDocumentContext::SaveOptions &options);
        ProjectFileError prepareSaveModel(opendspx::Model &model, const QString &destination = {});
        PreparedSave prepareSave(bool compress, const QString &destination = {});
        ProjectFileError writeSave(const QByteArray &data, const QString &path, SaveMode mode);

        ProjectDocumentContext *q_ptr;

        Core::FileLocker *fileLocker{};
        Document *document{};
        ProjectDocumentContext::Options options;
        QString defaultDocumentName;
        QString retainedReferenceFilePath;
        QString recoveryDirPath;
        QFile *documentLogDevice{};
        QLockFile *documentLogLock{};
        bool initializing = false;
        bool saving = false;
        bool preserveLog = false;

        QString recoveryDisplayName() const;
        bool writeRecoveryNameFile() const;
        bool lockRecoveryDirectory();
        void unlockRecoveryDirectory();
        void closeDocumentLogDevice();
        bool writeRecoverySnapshot() const;
        bool openDocumentLogDevice();
        bool setupDocumentLog();
        void cleanupDocumentLog();
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTDOCUMENTCONTEXT_P_H
