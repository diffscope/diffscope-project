#ifndef DIFFSCOPE_COREPLUGIN_CLOSESAVECHECKADDON_H
#define DIFFSCOPE_COREPLUGIN_CLOSESAVECHECKADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class CloseSaveCheckAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit CloseSaveCheckAddOn(QObject *parent = nullptr);
        ~CloseSaveCheckAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        bool handleCloseRequest() const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_CLOSESAVECHECKADDON_H
