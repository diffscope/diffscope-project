#include "directmanipulationaddon.h"

#include <QWDMHCore/DirectManipulationSystem.h>

namespace DirectManipulation::Internal {

    DirectManipulationAddOn::DirectManipulationAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    DirectManipulationAddOn::~DirectManipulationAddOn() = default;

    void DirectManipulationAddOn::initialize() {
        auto window = windowHandle()->window();
        QWDMH::DirectManipulationSystem::registerWindow(window);
        connectDockingView(window->property("leftDockingView").value<QObject *>());
        connectDockingView(window->property("rightDockingView").value<QObject *>());
        connectDockingView(window->property("topDockingView").value<QObject *>());
        connectDockingView(window->property("bottomDockingView").value<QObject *>());
    }

    void DirectManipulationAddOn::extensionsInitialized() {
    }

    void DirectManipulationAddOn::connectDockingView(QObject *dockingView) const {
        Q_ASSERT(dockingView);
        const QMetaObject *metaObject = dockingView->metaObject();
        connect(dockingView, SIGNAL(floatingWindowCreated(QQuickWindowQmlImpl*)), this, SLOT(registerWindow(QQuickWindowQmlImpl*)));
    }

    void DirectManipulationAddOn::registerWindow(QQuickWindowQmlImpl *window) {
        QWDMH::DirectManipulationSystem::registerWindow(window);
    }

}
