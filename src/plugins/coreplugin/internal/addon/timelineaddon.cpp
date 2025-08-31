#include "timelineaddon.h"

#include <QQmlComponent>
#include <QApplication>

#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/LongTime.h>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/projecttimeline.h>


namespace Core::Internal {
    TimelineAddOn::TimelineAddOn(QObject *parent) : IWindowAddOn(parent) {
    }
    TimelineAddOn::~TimelineAddOn() = default;
    void TimelineAddOn::initialize() {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "TimelineAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", iWin->actionContext());
        connect(iWin->projectTimeline(), &ProjectTimeline::positionChanged, this, [this] {
            Q_EMIT musicTimeTextChanged();
            Q_EMIT longTimeTextChanged();
        });
        connect(iWin->projectTimeline()->musicTimeline(), &SVS::MusicTimeline::changed, this, [this] {
            Q_EMIT musicTimeTextChanged();
            Q_EMIT longTimeTextChanged();
        });
    }
    void TimelineAddOn::extensionsInitialized() {
    }
    bool TimelineAddOn::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }

    QString TimelineAddOn::musicTimeText() const {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        return iWin->projectTimeline()->musicTimeline()->create(0, 0, iWin->projectTimeline()->position()).toString();
    }
    QString TimelineAddOn::longTimeText() const {
        auto iWin = windowHandle()->cast<IProjectWindow>();
        return SVS::LongTime(iWin->projectTimeline()
                                 ->musicTimeline()
                                 ->create(0, 0, iWin->projectTimeline()->position())
                                 .millisecond())
            .toString();
    }
    bool TimelineAddOn::showMusicTime() const {
        return m_showMusicTime;
    }
    void TimelineAddOn::setShowMusicTime(bool on) {
        if (on != m_showMusicTime) {
            m_showMusicTime = on;
            Q_EMIT showMusicTimeChanged();
        }
    }
    int TimelineAddOn::doubleClickInterval() {
        return QApplication::doubleClickInterval();
    }
}