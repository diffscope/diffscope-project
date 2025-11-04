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
        OpenSaveProjectFileScenario *openSaveProjectFileScenario;

        QDspx::Model model_TODO; // TODO

        void markSaved();
        QByteArray serializeDocument() const;
        bool deserializeDocument(const QByteArray &data);
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
