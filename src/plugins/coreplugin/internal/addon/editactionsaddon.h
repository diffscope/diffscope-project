#ifndef DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H
#define DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H

#include <qqmlintegration.h>

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class EditActionsAddOn : public IWindowAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit EditActionsAddOn(QObject *parent = nullptr);
        ~EditActionsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H