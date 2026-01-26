#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H

#include <opendspx/model.h>

#include <coreplugin/ProjectDocumentContext.h>

namespace Core {

    class ProjectDocumentContextPrivate {
        Q_DECLARE_PUBLIC(ProjectDocumentContext)
    public:
        ProjectDocumentContext *q_ptr;

        FileLocker *fileLocker{};
        DspxDocument *document{};
        OpenSaveProjectFileScenario *openSaveProjectFileScenario;

        void markSaved();
        QByteArray serializeDocument(bool *hasError = nullptr) const;

        bool deserializeAndInitializeDocument(const QByteArray &data);
        bool initializeDocument(const QDspx::Model &model, bool doCheck);
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
