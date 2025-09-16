#ifndef DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONADDON_H
#define DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONADDON_H

#include <QtQuick/private/qquickwindowmodule_p.h>

#include <CoreApi/windowinterface.h>

namespace DirectManipulation::Internal {

    class DirectManipulationAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit DirectManipulationAddOn(QObject *parent = nullptr);
        ~DirectManipulationAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

    private:


    private:
        void connectDockingView(QObject *dockingView) const;
        Q_SLOT static void registerWindow(QQuickWindowQmlImpl *window);

    };

}

#endif //DIFFSCOPE_DIRECT_MANIPULATION_DIRECTMANIPULATIONADDON_H
