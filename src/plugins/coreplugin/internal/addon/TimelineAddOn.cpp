#include "TimelineAddOn.h"

#include <algorithm>

#include <QApplication>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/LongTime.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/QuickInput.h>

namespace Core::Internal {
    TimelineAddOn::TimelineAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }
    TimelineAddOn::~TimelineAddOn() = default;
    void TimelineAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "TimelineAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        connect(windowInterface->projectTimeline(), &ProjectTimeline::positionChanged, this, [this] {
            Q_EMIT musicTimeTextChanged();
            Q_EMIT longTimeTextChanged();
            Q_EMIT tempoTextChanged();
            Q_EMIT timeSignatureTextChanged();
        });
        connect(windowInterface->projectTimeline()->musicTimeline(), &SVS::MusicTimeline::changed, this, [this] {
            Q_EMIT musicTimeTextChanged();
            Q_EMIT longTimeTextChanged();
            Q_EMIT tempoTextChanged();
            Q_EMIT timeSignatureTextChanged();
        });
    }
    void TimelineAddOn::extensionsInitialized() {
    }
    bool TimelineAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QString TimelineAddOn::musicTimeText() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        return windowInterface->projectTimeline()->musicTimeline()->create(0, 0, windowInterface->projectTimeline()->position()).toString();
    }
    QString TimelineAddOn::longTimeText() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        return SVS::LongTime(windowInterface->projectTimeline()
                                 ->musicTimeline()
                                 ->create(0, 0, windowInterface->projectTimeline()->position())
                                 .millisecond())
            .toString();
    }
    QString TimelineAddOn::tempoText() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto projectTimeline = windowInterface->projectTimeline();
        return QLocale().toString(projectTimeline->musicTimeline()->tempoAt(projectTimeline->position()));
    }
    QString TimelineAddOn::timeSignatureText() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto projectTimeline = windowInterface->projectTimeline();
        return projectTimeline->musicTimeline()->timeSignatureAt(projectTimeline->position()).toString();
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

    static inline QString absoluteMusicTimePromptText(const SVS::MusicTime &t) {
        if (t.tick() == 0) {
            return Core::Internal::TimelineAddOn::tr("measure %L1, beat %L2").arg(t.measure() + 1).arg(t.beat() + 1);
        }
        return Core::Internal::TimelineAddOn::tr("measure %L1, beat %L2, tick %L3").arg(t.measure() + 1).arg(t.beat() + 1).arg(t.tick());
    }
    static inline QString absoluteLongTimePromptText(const SVS::LongTime &t) {
        auto minuteText = Core::Internal::TimelineAddOn::tr("%Ln minute(s)", "absolute time", t.minute());
        auto secondText = Core::Internal::TimelineAddOn::tr("%Ln second(s)", "absolute time", t.second());
        auto millisecondText = Core::Internal::TimelineAddOn::tr("%Ln millisecond(s)", "absolute time", t.millisecond());
        if (t.minute() == 0 && t.second() == 0) {
            return millisecondText;
        }
        if (t.minute() == 0 && t.millisecond() == 0) {
            return secondText;
        }
        if (t.minute() == 0 && t.millisecond() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "absolute minute second").arg(minuteText, secondText);
        }
        if (t.minute() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "absolute second millisecond").arg(secondText, millisecondText);
        }
        if (t.millisecond() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "absolute minute second").arg(minuteText, secondText);
        }
        return Core::Internal::TimelineAddOn::tr("%1 %2 %3", "absolute minute second millisecond").arg(minuteText, secondText, millisecondText);
    }
    static inline QString relativeLongTimePromptText(const SVS::LongTime &t) {
        auto minuteText = Core::Internal::TimelineAddOn::tr("%Ln minute(s)", "relative time", t.minute());
        auto secondText = Core::Internal::TimelineAddOn::tr("%Ln second(s)", "relative time", t.second());
        auto millisecondText = Core::Internal::TimelineAddOn::tr("%Ln millisecond(s)", "relative time", t.millisecond());
        if (t.minute() == 0 && t.second() == 0) {
            return millisecondText;
        }
        if (t.minute() == 0 && t.millisecond() == 0) {
            return secondText;
        }
        if (t.minute() == 0 && t.millisecond() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "relative minute second").arg(minuteText, secondText);
        }
        if (t.minute() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "relative second millisecond").arg(secondText, millisecondText);
        }
        if (t.millisecond() == 0) {
            return Core::Internal::TimelineAddOn::tr("%1 %2", "relative minute second").arg(minuteText, secondText);
        }
        return Core::Internal::TimelineAddOn::tr("%1 %2 %3", "relative minute second millisecond").arg(minuteText, secondText, millisecondText);
    }
    static SVS::PersistentMusicTime quickJumpParseAbsoluteMusicTime(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        bool ok;
        auto t = timeline->musicTimeline()->create(text, &ok);
        if (!ok) {
            return {};
        }
        quickInput->setAcceptable(true);
        quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
        quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(absoluteMusicTimePromptText(t)));
        return t;
    }
    static SVS::PersistentMusicTime quickJumpParseRelativeMusicTime(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        auto s = QStringView(text);
        int p;
        if (s.startsWith('-')) {
            p = -1;
        } else if (s.startsWith('+')) {
            p = 1;
        } else {
            return {};
        }
        s = s.mid(1);
        // TODO
        return {};
    }
    static SVS::PersistentMusicTime quickJumpParseAbsoluteLongTime(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        auto s = QStringView(text);
        if (s.startsWith('^')) {
            s = s.mid(1);
        }
        if (s.trimmed().startsWith('-')) {
            return {};
        }
        bool ok;
        auto longTime = SVS::LongTime::fromString(s, &ok);
        if (!ok) {
            return {};
        }
        auto musicTime = timeline->musicTimeline()->create(longTime.totalMillisecond());
        quickInput->setAcceptable(!s.trimmed().isEmpty());
        quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
        quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(s.trimmed().isEmpty() ? Core::Internal::TimelineAddOn::tr("absolute time...") : Core::Internal::TimelineAddOn::tr("%1 (%2)").arg(absoluteLongTimePromptText(longTime), absoluteMusicTimePromptText(musicTime))));
        return musicTime;
    }
    static SVS::PersistentMusicTime quickJumpParseRelativeLongTime(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        auto s = QStringView(text);
        int p;
        if (s.startsWith('-')) {
            p = -1;
        } else if (s.startsWith('+')) {
            p = 1;
        } else {
            return {};
        }
        s = s.mid(1);
        if (s.startsWith('^')) {
            s = s.mid(1);
        }
        bool ok;
        auto longTime = SVS::LongTime::fromString(s, &ok);
        if (!ok) {
            return {};
        }
        auto promptTextPrefix = p == -1 ? Core::Internal::TimelineAddOn::tr("Move backward by %1") : Core::Internal::TimelineAddOn::tr("Move forward by %1");
        auto currentMusicTime = timeline->musicTimeline()->create(timeline->position());
        auto targetMillisecond = currentMusicTime.millisecond() + p * longTime.totalMillisecond();
        auto targetMusicTime = timeline->musicTimeline()->create(qMax(0.0, targetMillisecond));
        quickInput->setAcceptable(!s.trimmed().isEmpty());
        quickInput->setStatus(targetMillisecond < 0 ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::ControlType::CT_Normal);
        quickInput->setPromptText(
            promptTextPrefix.arg(s.trimmed().isEmpty() ? Core::Internal::TimelineAddOn::tr("absolute time...") : Core::Internal::TimelineAddOn::tr("%1 (to %2)").arg(relativeLongTimePromptText(longTime), absoluteMusicTimePromptText(targetMusicTime))) +
            (targetMillisecond < 0 ? Core::Internal::TimelineAddOn::tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : "")
        );
        return targetMusicTime;
    }
    static SVS::PersistentMusicTime quickJumpParseMoveCommand(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        if (text == "\\" || text == "\u3001") {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->rangeHint() - 1);
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("the end of project (%1)").arg(absoluteMusicTimePromptText(t))));
            return t;
        }
        auto isLeftSquareBracket = [](QChar c) {
            return c == '[' || c == u'\u3010';
        };
        if (text.length() == 1 && std::ranges::all_of(text, isLeftSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()).previousMeasure();
            bool adjusted = false;
            if (!t.isValid()) {
                adjusted = true;
                t = timeline->musicTimeline()->create(0, 0, 0);
            }
            quickInput->setAcceptable(true);
            quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("previous measure (%1)").arg(absoluteMusicTimePromptText(t))) + (adjusted ? Core::Internal::TimelineAddOn::tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
            return t;
        }
        if (text.length() == 2 && std::ranges::all_of(text, isLeftSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()).previousBeat();
            bool adjusted = false;
            if (!t.isValid()) {
                adjusted = true;
                t = timeline->musicTimeline()->create(0, 0, 0);
            }
            quickInput->setAcceptable(true);
            quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("previous beat (%1)").arg(absoluteMusicTimePromptText(t))) + (adjusted ? Core::Internal::TimelineAddOn::tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
            return t;
        }
        if (text.length() == 3 && std::ranges::all_of(text, isLeftSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()) - 1;
            bool adjusted = false;
            if (!t.isValid()) {
                adjusted = true;
                t = timeline->musicTimeline()->create(0, 0, 0);
            }
            quickInput->setAcceptable(true);
            quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("previous tick (%1)").arg(absoluteMusicTimePromptText(t))) + (adjusted ? Core::Internal::TimelineAddOn::tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
            return t;
        }
        auto isRightSquareBracket = [](QChar c) {
            return c == ']' || c == u'\u3011';
        };
        if (text.length() == 1 && std::ranges::all_of(text, isRightSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()).nextMeasure();
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("next measure (%1)").arg(absoluteMusicTimePromptText(t))));
            return t;
        }
        if (text.length() == 2 && std::ranges::all_of(text, isRightSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()).nextBeat();
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("next beat (%1)").arg(absoluteMusicTimePromptText(t))));
            return t;
        }
        if (text.length() == 3 && std::ranges::all_of(text, isRightSquareBracket)) {
            auto t = timeline->musicTimeline()->create(0, 0, timeline->position()) + 1;
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Go to %1").arg(Core::Internal::TimelineAddOn::tr("next tick (%1)").arg(absoluteMusicTimePromptText(t))));
            return t;
        }
        return {};
    }
    static SVS::PersistentMusicTime quickJumpParseEasterEggs(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        if (text.toLower() == "yajuusenpai") {
            return quickJumpParseAbsoluteLongTime("1:14.514", quickInput, timeline);
        }
        if (text.toLower() == "the answer to the ultimate question of life, the universe, and everything") {
            return quickJumpParseAbsoluteMusicTime("42", quickInput, timeline);
        }
        if (text.toLower() == "crindzebra sjimo") {
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            quickInput->setPromptText(QStringLiteral("Do you also want to tell your own story through music?"));
            return timeline->musicTimeline()->create(0, 0, 16423);
        }
        if (text.toLower() == "9bang15\u4fbf\u58eb") {
            quickInput->setAcceptable(true);
            quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            return quickJumpParseAbsoluteLongTime("9.15", quickInput, timeline);
        }
        return {};
    }

    static SVS::PersistentMusicTime (*quickJumpParseFunctions[])(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) = {
        quickJumpParseAbsoluteMusicTime,
        quickJumpParseRelativeMusicTime,
        quickJumpParseAbsoluteLongTime,
        quickJumpParseRelativeLongTime,
        quickJumpParseMoveCommand,
        quickJumpParseEasterEggs,
    };

    static SVS::PersistentMusicTime quickJumpParse(const QString &text, QuickInput *quickInput, const ProjectTimeline *timeline) {
        quickInput->setPromptText({});
        if (text.trimmed().isEmpty()) {
            quickInput->setAcceptable(false);
            quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Type \"?\" to view tips"));
            return {};
        }
        if (text == "?" || text == "\uff1f") {
            quickInput->setAcceptable(false);
            quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            quickInput->setPromptText("aaa"); // TODO
            return {};
        }
        for (auto f : quickJumpParseFunctions) {
            auto t = f(text, quickInput, timeline);
            if (t.isValid())
                return t;
        }
        quickInput->setAcceptable(false);
        if (quickInput->promptText().isEmpty()) {
            quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Error);
            quickInput->setPromptText(Core::Internal::TimelineAddOn::tr("Invalid format"));
        }
        return {};
    }

    void TimelineAddOn::execQuickJump(const QString &initialText) const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QuickInput quickInput;
        quickInput.setPlaceholderText(tr("Jump to"));
        quickInput.setPromptText(tr("Type \"?\" to view tips"));
        quickInput.setText(initialText);
        quickInput.setWindowHandle(windowInterface);
        connect(&quickInput, &QuickInput::textChanged, this, [&, this](const QString &text) {
            quickJumpParse(text, &quickInput, windowInterface->projectTimeline());
        });
        connect(&quickInput, &QuickInput::attemptingAcceptButFailed, this, [&, this]() {
            quickInput.setStatus(SVS::SVSCraft::CT_Error);
            if (quickInput.text().trimmed().isEmpty()) {
                quickInput.setPromptText(tr("Input should not be empty"));
            } else {
                quickInput.setPromptText(tr("Invalid format"));
            }
        });
        quickJumpParse(quickInput.text(), &quickInput, windowInterface->projectTimeline());
        auto ret = quickInput.exec();
        if (!ret.isValid())
            return;
        auto t = quickJumpParse(quickInput.text(), &quickInput, windowInterface->projectTimeline());
        if (!t.isValid())
            return;
        windowInterface->projectTimeline()->goTo(t.totalTick());
    }
    void TimelineAddOn::goToStart() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        l->goTo(0);
    }
    void TimelineAddOn::goToPreviousMeasure() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t.previousMeasure();
        if (!t.isValid())
            t = l->musicTimeline()->create(0, 0, 0);
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::goToPreviousBeat() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t.previousBeat();
        if (!t.isValid())
            t = l->musicTimeline()->create(0, 0, 0);
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::goToPreviousTick() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t - 1;
        if (!t.isValid())
            t = l->musicTimeline()->create(0, 0, 0);
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::goToEnd() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        l->goTo(l->rangeHint() - 1);
    }
    void TimelineAddOn::goToNextMeasure() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t.nextMeasure();
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::goToNextBeat() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t.nextBeat();
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::goToNextTick() const {
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        auto t = l->musicTimeline()->create(0, 0, l->position());
        t = t + 1;
        l->goTo(t.totalTick());
    }
    void TimelineAddOn::resetProjectTimeRange() const {
        // TODO
        auto l = windowHandle()->cast<ProjectWindowInterface>()->projectTimeline();
        l->setRangeHint(qMax(l->position(), l->lastPosition()) + 1);
    }
}
