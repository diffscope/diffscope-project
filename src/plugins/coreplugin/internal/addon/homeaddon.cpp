#include "homeaddon.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/homewindowinterface.h>

namespace Core::Internal {
    HomeAddOn::HomeAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }
    HomeAddOn::~HomeAddOn() = default;
    void HomeAddOn::initialize() {
    }
    void HomeAddOn::extensionsInitialized() {
    }
    bool HomeAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}
