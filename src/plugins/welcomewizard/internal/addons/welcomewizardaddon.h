#ifndef DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H
#define DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H

#include <qqmlintegration.h>

#include <QQuickWindow>

#include <CoreApi/windowinterface.h>

namespace WelcomeWizard {

    class WelcomeWizardPlugin;

    class WelcomeWizardAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QQuickWindow *window READ window CONSTANT)
    public:
        explicit WelcomeWizardAddOn(QObject *parent = nullptr);
        ~WelcomeWizardAddOn() override;

        static void setPlugin(WelcomeWizardPlugin *plugin);
        static QQuickWindow *window();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE static void execWelcomeWizard();
    };

}

#endif //DIFFSCOPE_WELCOMEWIZARD_WELCOMEWIZARDADDON_H
