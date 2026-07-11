#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H

#include <opendspx/model.h>

#include <coreplugin/ProjectDocumentContext.h>

class QFile;
class QLockFile;

namespace Core {

    class ProjectDocumentContextPrivate {
        Q_DECLARE_PUBLIC(ProjectDocumentContext)
    public:
        ProjectDocumentContext *q_ptr;

        FileLocker *fileLocker{};
        DspxDocument *document{};
        OpenSaveProjectFileScenario *openSaveProjectFileScenario;
        QString defaultDocumentName;
        QString uuidString;
        QString recoveryDirPath;
        QFile *documentLogDevice{};
        QLockFile *documentLogLock{};
        bool documentLogEnabled{};

        void markSaved();
        QByteArray serializeDocument(bool *hasError = nullptr) const;

        bool deserializeAndInitializeDocument(const QByteArray &data);
        bool initializeDocument(const opendspx::Model &model, bool doCheck);
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
