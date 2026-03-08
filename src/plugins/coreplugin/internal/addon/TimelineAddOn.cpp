#include "TimelineAddOn.h"

#include <algorithm>

#include <QApplication>
#include <QQmlComponent>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/LongTime.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicModeInfo.h>
#include <SVSCraftCore/MusicMode.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>

#include <transactional/TransactionController.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/QuickInput.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/internal/KeySignatureAtSpecifiedPositionHelper.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcTimelineAddOn, "diffscope.core.timelineaddon")

    static QString getKeySignatureText(int mode, int tonality, int accidentalType) {
        static auto map = [] {
            QMap<int, QString> map;
            for (const auto &[musicMode, name] : SVS::MusicModeInfo::getBuiltInMusicModeInfoList()) {
                map.insert(musicMode.mask(), name);
            }
            return map;
        }();
        auto modeName = map.value(mode);
        if (modeName.isEmpty()) {
            modeName = Core::Internal::TimelineAddOn::tr("Custom Mode");
        }
        if (mode == 0) { // Atonal
            return modeName;
        }
        auto keyName = SVS::MusicPitch(tonality).toString(static_cast<SVS::MusicPitch::Accidental>(accidentalType));
        keyName = keyName.slice(0, keyName.length() - 1);
        return QString("%1 %2").arg(keyName, modeName);
    }

    TimelineAddOn::TimelineAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        m_keySignatureAtSpecifiedPositionHelper = new KeySignatureAtSpecifiedPositionHelper(this);

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
        m_keySignatureAtSpecifiedPositionHelper->setPosition(windowInterface->projectTimeline()->position());
        m_keySignatureAtSpecifiedPositionHelper->setKeySignatureSequence(windowInterface->projectDocumentContext()->document()->model()->timeline()->keySignatures());
        connect(windowInterface->projectTimeline(), &ProjectTimeline::positionChanged, this, [this](int position) {
            m_keySignatureAtSpecifiedPositionHelper->setPosition(position);
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
        connect(windowInterface->projectDocumentContext()->document()->model()->timeline(), &dspx::Timeline::loopEnabledChanged, this, &TimelineAddOn::loopEnabledChanged);
        connect(m_keySignatureAtSpecifiedPositionHelper, &KeySignatureAtSpecifiedPositionHelper::modeChanged, this, &TimelineAddOn::keySignatureTextChanged);
        connect(m_keySignatureAtSpecifiedPositionHelper, &KeySignatureAtSpecifiedPositionHelper::tonalityChanged, this, &TimelineAddOn::keySignatureTextChanged);
        connect(m_keySignatureAtSpecifiedPositionHelper, &KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged, this, &TimelineAddOn::keySignatureTextChanged);
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
        return projectTimeline->musicTimeline()->timeSignatureAt(projectTimeline->musicTimeline()->create(0, 0, projectTimeline->position()).measure()).toString();
    }
    QString TimelineAddOn::keySignatureText() const {
        return getKeySignatureText(m_keySignatureAtSpecifiedPositionHelper->mode(), m_keySignatureAtSpecifiedPositionHelper->tonality(), m_keySignatureAtSpecifiedPositionHelper->accidentalType());
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

    bool TimelineAddOn::isLoopEnabled() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        return windowInterface->projectDocumentContext()->document()->model()->timeline()->isLoopEnabled();
    }

    void TimelineAddOn::setLoopEnabled(bool enabled) {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        qCInfo(lcTimelineAddOn) << "Toggle loop:" << enabled;
        windowInterface->projectDocumentContext()->document()->transactionController()->beginScopedTransaction(enabled ? tr("Enabling loop") : tr("Disabling loop"), [=] {
            auto dspxTimeline = windowInterface->projectDocumentContext()->document()->model()->timeline();
            dspxTimeline->setLoopEnabled(enabled);
            return true;
        }, [=, this] {
            qCCritical(lcTimelineAddOn()) << "Failed to edit loop in exclusive transaction";
            Q_EMIT loopEnabledChanged();
        });
    }

    class QuickJumpParser : public QObject {
        Q_OBJECT
        
    public:
        explicit QuickJumpParser(QuickInput *quickInput, const ProjectTimeline *timeline, QObject *parent = nullptr)
            : QObject(parent) , m_quickInput(quickInput) , m_timeline(timeline)
        {
            connect(m_quickInput, &QuickInput::textChanged, this, &QuickJumpParser::onTextChanged);
            // 立即解析初始文本
            onTextChanged(m_quickInput->text());
        }
        
        SVS::PersistentMusicTime result() const {
            return m_result;
        }

    private:
        void onTextChanged(const QString &text) {
            m_result = parse(text);
        }

        SVS::PersistentMusicTime parse(const QString &text) const {
            m_quickInput->setPromptText({});
            if (text.trimmed().isEmpty()) {
                m_quickInput->setAcceptable(false);
                m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
                m_quickInput->setPromptText(tr("Type \"?\" to view tips"));
                return {};
            }
            if (text == "?" || text == "\uff1f") {
                m_quickInput->setAcceptable(false);
                m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
                m_quickInput->setPromptText(qtTrId("org.diffscope.core.timelineaddon.quick_jump_help"));
                return {};
            }
            for (auto method : s_parseMethods) {
                auto result = (this->*method)(text);
                if (result.isValid())
                    return result;
            }
            m_quickInput->setAcceptable(false);
            if (m_quickInput->promptText().isEmpty()) {
                m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Error);
                m_quickInput->setPromptText(tr("Invalid format"));
            }
            return {};
        }
        
        SVS::PersistentMusicTime parseAbsoluteMusicTime(const QString &text) const {
            bool ok;
            auto t = m_timeline->musicTimeline()->create(text, &ok);
            if (!ok) {
                return {};
            }
            m_quickInput->setAcceptable(true);
            m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            m_quickInput->setPromptText(tr("Go to %1").arg(formatAbsoluteMusicTimePrompt(t)));
            return t;
        }
        
        SVS::PersistentMusicTime parseRelativeMusicTime(const QString &text) const {
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
            bool ok;
            auto musicTimeOffset = SVS::MusicTimeOffset::fromString(s, &ok);
            if (!ok) {
                return {};
            }
            auto promptTextPrefix = p == -1 ? tr("Move backward by %1 (to %2)") : tr("Move forward by %1 (to %2)");
            auto promptTextWhenEmpty = p == -1 ? tr("Move backward by music time...") : tr("Move forward by music time...");
            auto targetTick = m_timeline->position() + p * musicTimeOffset.totalTick();
            auto targetMusicTime = m_timeline->musicTimeline()->create(0, 0, qMax(0, targetTick));
            m_quickInput->setAcceptable(!s.trimmed().isEmpty());
            m_quickInput->setStatus(targetTick < 0 ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::ControlType::CT_Normal);
            m_quickInput->setPromptText(
                s.trimmed().isEmpty() ? promptTextWhenEmpty : promptTextPrefix.arg(formatRelativeMusicTimePrompt(musicTimeOffset), formatAbsoluteMusicTimePrompt(targetMusicTime)) +
                (targetTick < 0 ? tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : "")
            );
            return targetMusicTime;
            return {};
        }
        
        SVS::PersistentMusicTime parseAbsoluteLongTime(const QString &text) const {
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
            auto musicTime = m_timeline->musicTimeline()->create(longTime.totalMillisecond());
            m_quickInput->setAcceptable(!s.trimmed().isEmpty());
            m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
            m_quickInput->setPromptText(s.trimmed().isEmpty() ? tr("Go to absolute time...") : tr("Go to %1").arg(tr("%1 (%2)").arg(formatAbsoluteLongTimePrompt(longTime), formatAbsoluteMusicTimePrompt(musicTime))));
            return musicTime;
        }
        
        SVS::PersistentMusicTime parseRelativeLongTime(const QString &text) const {
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
            auto promptTextPrefix = p == -1 ? tr("Move backward by %1 (to %2)") : tr("Move forward by %1 (to %2)");
            auto promptTextWhenEmpty = p == -1 ? tr("Move backward by absolute time...") : tr("Move forward by absolute time...");
            auto currentMusicTime = m_timeline->musicTimeline()->create(m_timeline->position());
            auto targetMillisecond = currentMusicTime.millisecond() + p * longTime.totalMillisecond();
            auto targetMusicTime = m_timeline->musicTimeline()->create(qMax(0.0, targetMillisecond));
            m_quickInput->setAcceptable(!s.trimmed().isEmpty());
            m_quickInput->setStatus(targetMillisecond < 0 ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::ControlType::CT_Normal);
            m_quickInput->setPromptText(
                s.trimmed().isEmpty() ? promptTextWhenEmpty : promptTextPrefix.arg(formatRelativeLongTimePrompt(longTime), formatAbsoluteMusicTimePrompt(targetMusicTime)) +
                (targetMillisecond < 0 ? tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : "")
            );
            return targetMusicTime;
        }
        
        SVS::PersistentMusicTime parseMoveCommand(const QString &text) const {
            if (text == "\\" || text == "\u3001") {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->rangeHint() - 1);
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("the end of project (%1)").arg(formatAbsoluteMusicTimePrompt(t))));
                return t;
            }
            auto isLeftSquareBracket = [](QChar c) {
                return c == '[' || c == u'\u3010';
            };
            if (text.length() == 1 && std::ranges::all_of(text, isLeftSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()).previousMeasure();
                bool adjusted = false;
                if (!t.isValid()) {
                    adjusted = true;
                    t = m_timeline->musicTimeline()->create(0, 0, 0);
                }
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("previous measure (%1)").arg(formatAbsoluteMusicTimePrompt(t))) + (adjusted ? tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
                return t;
            }
            if (text.length() == 2 && std::ranges::all_of(text, isLeftSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()).previousBeat();
                bool adjusted = false;
                if (!t.isValid()) {
                    adjusted = true;
                    t = m_timeline->musicTimeline()->create(0, 0, 0);
                }
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("previous beat (%1)").arg(formatAbsoluteMusicTimePrompt(t))) + (adjusted ? tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
                return t;
            }
            if (text.length() == 3 && std::ranges::all_of(text, isLeftSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()) - 1;
                bool adjusted = false;
                if (!t.isValid()) {
                    adjusted = true;
                    t = m_timeline->musicTimeline()->create(0, 0, 0);
                }
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(adjusted ? SVS::SVSCraft::CT_Warning : SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("previous tick (%1)").arg(formatAbsoluteMusicTimePrompt(t))) + (adjusted ? tr("\nThe time offset exceeds the boundary and has been adjusted to zero") : ""));
                return t;
            }
            auto isRightSquareBracket = [](QChar c) {
                return c == ']' || c == u'\u3011';
            };
            if (text.length() == 1 && std::ranges::all_of(text, isRightSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()).nextMeasure();
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("next measure (%1)").arg(formatAbsoluteMusicTimePrompt(t))));
                return t;
            }
            if (text.length() == 2 && std::ranges::all_of(text, isRightSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()).nextBeat();
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("next beat (%1)").arg(formatAbsoluteMusicTimePrompt(t))));
                return t;
            }
            if (text.length() == 3 && std::ranges::all_of(text, isRightSquareBracket)) {
                auto t = m_timeline->musicTimeline()->create(0, 0, m_timeline->position()) + 1;
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::CT_Normal);
                m_quickInput->setPromptText(tr("Go to %1").arg(tr("next tick (%1)").arg(formatAbsoluteMusicTimePrompt(t))));
                return t;
            }
            return {};
        }
        
        SVS::PersistentMusicTime parseEasterEggs(const QString &text) const {
            if (text.toLower() == "yajuusenpai") {
                return parseAbsoluteLongTime("1:14.514");
            }
            if (text.toLower() == "the answer to the ultimate question of life, the universe, and everything") {
                return parseAbsoluteMusicTime("42");
            }
            if (text.toLower() == "crindzebra sjimo") {
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
                m_quickInput->setPromptText(QStringLiteral("Do you also want to tell your own story through music?"));
                return m_timeline->musicTimeline()->create(0, 0, 16423);
            }
            if (text.toLower() == "9bang15\u4fbf\u58eb") {
                m_quickInput->setAcceptable(true);
                m_quickInput->setStatus(SVS::SVSCraft::ControlType::CT_Normal);
                return parseAbsoluteLongTime("9.15");
            }
            return {};
        }
        
        static QString formatAbsoluteMusicTimePrompt(const SVS::MusicTime &t) {
            if (t.tick() == 0) {
                return tr("measure %L1, beat %L2").arg(t.measure() + 1).arg(t.beat() + 1);
            }
            return tr("measure %L1, beat %L2, tick %L3").arg(t.measure() + 1).arg(t.beat() + 1).arg(t.tick());
        }

        static QString formatRelativeMusicTimePrompt(const SVS::MusicTimeOffset &t) {
            auto quarterNoteText = tr("%Ln quarter note(s)", "relative time", t.quarterNote());
            auto tickText = tr("%Ln tick(s)", "relative time", t.tick());
            if (t.quarterNote() == 0) {
                return tickText;
            }
            if (t.tick() == 0) {
                return quarterNoteText;
            }
            return tr("%1 %2", "relative quarter-note tick").arg(quarterNoteText, tickText);
        }
        
        static QString formatAbsoluteLongTimePrompt(const SVS::LongTime &t) {
            auto minuteText = tr("%Ln minute(s)", "absolute time", t.minute());
            auto secondText = tr("%Ln second(s)", "absolute time", t.second());
            auto millisecondText = tr("%Ln millisecond(s)", "absolute time", t.millisecond());
            if (t.minute() == 0 && t.second() == 0) {
                return millisecondText;
            }
            if (t.minute() == 0 && t.millisecond() == 0) {
                return secondText;
            }
            if (t.minute() == 0 && t.millisecond() == 0) {
                return tr("%1 %2", "absolute minute second").arg(minuteText, secondText);
            }
            if (t.minute() == 0) {
                return tr("%1 %2", "absolute second millisecond").arg(secondText, millisecondText);
            }
            if (t.millisecond() == 0) {
                return tr("%1 %2", "absolute minute second").arg(minuteText, secondText);
            }
            return tr("%1 %2 %3", "absolute minute second millisecond").arg(minuteText, secondText, millisecondText);
        }
        
        static QString formatRelativeLongTimePrompt(const SVS::LongTime &t) {
            auto minuteText = tr("%Ln minute(s)", "relative time", t.minute());
            auto secondText = tr("%Ln second(s)", "relative time", t.second());
            auto millisecondText = tr("%Ln millisecond(s)", "relative time", t.millisecond());
            if (t.minute() == 0 && t.second() == 0) {
                return millisecondText;
            }
            if (t.minute() == 0 && t.millisecond() == 0) {
                return secondText;
            }
            if (t.minute() == 0 && t.millisecond() == 0) {
                return tr("%1 %2", "relative minute second").arg(minuteText, secondText);
            }
            if (t.minute() == 0) {
                return tr("%1 %2", "relative second millisecond").arg(secondText, millisecondText);
            }
            if (t.millisecond() == 0) {
                return tr("%1 %2", "relative minute second").arg(minuteText, secondText);
            }
            return tr("%1 %2 %3", "relative minute second millisecond").arg(minuteText, secondText, millisecondText);
        }
        
        using ParseMethod = SVS::PersistentMusicTime (QuickJumpParser::*)(const QString &) const;
        static constexpr ParseMethod s_parseMethods[] = {
            &QuickJumpParser::parseAbsoluteMusicTime,
            &QuickJumpParser::parseRelativeMusicTime,
            &QuickJumpParser::parseAbsoluteLongTime,
            &QuickJumpParser::parseRelativeLongTime,
            &QuickJumpParser::parseMoveCommand,
            &QuickJumpParser::parseEasterEggs,
        };
        
        QuickInput *m_quickInput;
        const ProjectTimeline *m_timeline;
        SVS::PersistentMusicTime m_result;
    };

    void TimelineAddOn::execQuickJump(const QString &initialText) const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        QuickInput quickInput;
        quickInput.setPlaceholderText(tr("Jump to"));
        quickInput.setText(initialText);
        quickInput.setWindowHandle(windowInterface);
        
        // 创建解析器，自动连接并开始解析
        QuickJumpParser parser(&quickInput, windowInterface->projectTimeline(), &quickInput);
        
        connect(&quickInput, &QuickInput::attemptingAcceptButFailed, this, [&]() {
            quickInput.setStatus(SVS::SVSCraft::CT_Error);
            if (quickInput.text().trimmed().isEmpty()) {
                quickInput.setPromptText(tr("Input should not be empty"));
            } else {
                quickInput.setPromptText(tr("Invalid format"));
            }
        });
        
        auto ret = quickInput.exec();
        if (!ret.isValid())
            return;
            
        auto t = parser.result();
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
        windowHandle()->cast<ProjectWindowInterface>()->boundTimelineRangeHint();
    }
}

#include "TimelineAddOn.moc"
