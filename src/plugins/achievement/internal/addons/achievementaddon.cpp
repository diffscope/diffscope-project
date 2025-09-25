#include "achievementaddon.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/actionwindowinterfacebase.h>

namespace Achievement {
    AchievementAddOn::AchievementAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    AchievementAddOn::~AchievementAddOn() = default;

    static QQuickWindow *m_window = nullptr;

    QQuickWindow *AchievementAddOn::window() {
        return m_window;
    }
    void AchievementAddOn::setWindow(QQuickWindow *w) {
        m_window = w;
    }

    void AchievementAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Achievement", "AchievementActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)}
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void AchievementAddOn::extensionsInitialized() {
    }

    bool AchievementAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}
