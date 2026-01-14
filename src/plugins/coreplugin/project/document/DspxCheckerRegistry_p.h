#ifndef DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_P_H

#include <coreplugin/DspxCheckerRegistry.h>

namespace Core {
    class DspxCheckerRegistryPrivate {
        Q_DECLARE_PUBLIC(DspxCheckerRegistry)
    public:
        DspxCheckerRegistry *q_ptr;
        QList<IDspxChecker *> checkers;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_P_H
