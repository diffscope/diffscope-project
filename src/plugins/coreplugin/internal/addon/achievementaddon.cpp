#include "achievementaddon.h"

#include <QQmlComponent>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iactionwindowbase.h>

namespace Achievement {
    AchievementAddOn::AchievementAddOn(QObject *parent) : Core::IWindowAddOn(parent) {
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
        auto iWin = windowHandle()->cast<Core::IActionWindowBase>();
        QQmlComponent component(Core::PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "AchievementActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)}
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