#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H

#include <coreplugin/DspxDocument.h>

namespace Core {
    class DspxDocumentPrivate {
        Q_DECLARE_PUBLIC(DspxDocument)
    public:
        DspxDocument *q_ptr;
        dspx::Model *model;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
