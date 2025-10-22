#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H

#include <coreplugin/projectdocumentcontext.h>

namespace Core {

    class ProjectDocumentContextPrivate {
        Q_DECLARE_PUBLIC(ProjectDocumentContext)
    public:
        ProjectDocumentContext *q_ptr;

        FileLocker *fileLocker{};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_P_H
