#ifndef DIFFSCOPE_COREPLUGIN_INSERTITEMADDON_H
#define DIFFSCOPE_COREPLUGIN_INSERTITEMADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class InsertItemAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit InsertItemAddOn(QObject *parent = nullptr);
        ~InsertItemAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_INSERTITEMADDON_H