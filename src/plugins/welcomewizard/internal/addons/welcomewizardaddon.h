#ifndef DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H
#define DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H

#include <CoreApi/iwindow.h>
#include <qqmlintegration.h>

#include <QQuickWindow>

namespace WelcomeWizard {

    class WelcomeWizardAddOn : public Core::IWindowAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QQuickWindow *window READ window CONSTANT)
   public:
        explicit WelcomeWizardAddOn(QObject *parent = nullptr);
        ~WelcomeWizardAddOn() override;

        static QQuickWindow *window();
        static void setWindow(QQuickWindow *w);

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H