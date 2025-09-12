#include "achievementaddon.h"

#include <QQmlComponent>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iactionwindowbase.h>

namespace Achievement {
    AchievementAddOn::AchievementAddOn(QObject *parent) : Core::IWindowAddOn(parent) {
    }

    AchievementAddOn::~AchievementAddOn() = default;

    void AchievementAddOn::initialize() {
        auto iWin = windowHandle()->cast<Core::IActionWindowBase>();
        QQmlComponent component(Core::PluginDatabase::qmlEngine(), "DiffScope.Achievement", "AchievementActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({

        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());
    }

    void AchievementAddOn::extensionsInitialized() {
    }

    bool AchievementAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}