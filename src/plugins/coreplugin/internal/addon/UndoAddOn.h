#ifndef DIFFSCOPE_COREPLUGIN_UNDOADDON_H
#define DIFFSCOPE_COREPLUGIN_UNDOADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class UndoAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit UndoAddOn(QObject *parent = nullptr);
        ~UndoAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_UNDOADDON_H
