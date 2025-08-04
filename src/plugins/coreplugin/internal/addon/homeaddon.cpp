#include "homeaddon.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/icore.h>
#include <coreplugin/ihomewindow.h>

namespace Core::Internal {
    HomeAddOn::HomeAddOn(QObject *parent) : IWindowAddOn(parent) {
    }
    HomeAddOn::~HomeAddOn() = default;
    void HomeAddOn::initialize() {
    }
    void HomeAddOn::extensionsInitialized() {
    }
    bool HomeAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}