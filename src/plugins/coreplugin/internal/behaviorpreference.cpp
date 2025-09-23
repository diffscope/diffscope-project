#include "behaviorpreference.h"

#include <QApplication>
#include <QSettings>

#include <CoreApi/runtimeInterface.h>

namespace Core::Internal {

    class BehaviorPreferencePrivate {
    public:
        bool initialized{};

        BehaviorPreference::StartupBehavior startupBehavior{};
        bool useSystemLanguage{};
        QString localeName{};
        bool hasNotificationSoundAlert{};
        int notificationAutoHideTimeout{};
        int commandPaletteHistoryCount{};
        BehaviorPreference::ProxyOption proxyOption{};
        BehaviorPreference::ProxyType proxyType{};
        QString proxyHostname{};
        quint16 proxyPort{};
        bool proxyHasAuthentication{};
        QString proxyUsername{};
        QString proxyPassword{};
        bool autoCheckForUpdates{};
        BehaviorPreference::UpdateOption updateOption{};
        bool useCustomFont{};
        QString fontFamily{};
        QString fontStyle{};
        BehaviorPreference::UIBehavior uiBehavior{};
        BehaviorPreference::GraphicsBehavior graphicsBehavior{};
        bool animationEnabled{};
        double animationSpeedRatio{};
        bool timeIndicatorBackgroundVisible{};
        BehaviorPreference::TimeIndicatorInteractionBehavior timeIndicatorClickAction{};
        BehaviorPreference::TimeIndicatorInteractionBehavior timeIndicatorDoubleClickAction{};
        BehaviorPreference::TimeIndicatorInteractionBehavior timeIndicatorPressAndHoldAction{};
        bool timeIndicatorTextFineTuneEnabled{};
        bool timeIndicatorShowSliderOnHover{};
    };

    static BehaviorPreference *m_instance = nullptr;

    BehaviorPreference::BehaviorPreference(QObject *parent) : QObject(parent), d_ptr(new BehaviorPreferencePrivate) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }
    BehaviorPreference::~BehaviorPreference() {
        m_instance = nullptr;
    }

    BehaviorPreference *BehaviorPreference::instance() {
        return m_instance;
    }

    static constexpr char settingCategoryC[] = "Core::BehaviorPreference";

    void BehaviorPreference::load() {
        Q_D(BehaviorPreference);
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(settingCategoryC);
        d->startupBehavior = settings->value("startupBehavior", QVariant::fromValue(SB_CloseHomeWindowAfterOpeningProject)).value<StartupBehavior>();
        emit startupBehaviorChanged();
        d->useSystemLanguage = settings->value("useSystemLanguage", true).toBool();
        emit useSystemLanguageChanged();
        d->localeName = settings->value("localeName", QLocale().name()).toString();
        emit localeNameChanged();
        d->hasNotificationSoundAlert = settings->value("hasNotificationSoundAlert", false).toBool();
        emit hasNotificationSoundAlertChanged();
        d->notificationAutoHideTimeout = settings->value("notificationAutoHideTimeout", 5000).toInt();
        emit notificationAutoHideTimeoutChanged();
        d->commandPaletteHistoryCount = settings->value("commandPaletteHistoryCount", 16).toInt();
        emit commandPaletteHistoryCountChanged();
        d->proxyOption = settings->value("proxyOption", QVariant::fromValue(PO_System)).value<ProxyOption>();
        emit proxyOptionChanged();
        d->proxyType = settings->value("proxyType", QVariant::fromValue(PT_Socks5)).value<ProxyType>();
        emit proxyTypeChanged();
        d->proxyHostname = settings->value("proxyHostname").toString();
        emit proxyHostnameChanged();
        d->proxyPort = settings->value("proxyPort").value<quint16>();
        emit proxyPortChanged();
        d->proxyHasAuthentication = settings->value("proxyHasAuthentication", false).toBool();
        emit proxyHasAuthenticationChanged();
        d->proxyUsername = settings->value("proxyUsername").toString();
        emit proxyUsernameChanged();
        d->proxyPassword = settings->value("proxyPassword").toString();
        emit proxyPasswordChanged();
        d->autoCheckForUpdates = settings->value("autoCheckForUpdates", true).toBool();
        emit autoCheckForUpdatesChanged();
        d->updateOption = settings->value("updateOption", UO_Stable).value<UpdateOption>();
        emit updateOptionChanged();
        d->useCustomFont = settings->value("useCustomFont", false).toBool();
        emit useCustomFontChanged();
        d->fontFamily = settings->value("fontFamily", QApplication::font().family()).toString();
        emit fontFamilyChanged();
        d->fontStyle = settings->value("fontStyle", QApplication::font().style()).toString();
        emit fontStyleChanged();
        d->uiBehavior = settings->value("uiBehavior", QVariant::fromValue(UB_Frameless | UB_MergeMenuAndTitleBar)).value<UIBehavior>();
        emit uiBehaviorChanged();
        d->graphicsBehavior = settings->value("graphicsBehavior", QVariant::fromValue(GB_Hardware |GB_Antialiasing)).value<GraphicsBehavior>();
        emit graphicsBehaviorChanged();
        d->animationEnabled = settings->value("animationEnabled", true).toBool();
        emit animationEnabledChanged();
        d->animationSpeedRatio = settings->value("animationSpeedRatio", 1.0).toDouble();
        emit animationSpeedRatioChanged();
        d->timeIndicatorBackgroundVisible = settings->value("timeIndicatorBackgroundVisible", true).toBool();
        emit timeIndicatorBackgroundVisibleChanged();
        d->timeIndicatorClickAction = settings->value("timeIndicatorClickAction", QVariant::fromValue(TIIB_ToggleFormat)).value<TimeIndicatorInteractionBehavior>();
        emit timeIndicatorClickActionChanged();
        d->timeIndicatorDoubleClickAction = settings->value("timeIndicatorDoubleClickAction", QVariant::fromValue(TIIB_ShowQuickJump)).value<TimeIndicatorInteractionBehavior>();
        emit timeIndicatorDoubleClickActionChanged();
        d->timeIndicatorPressAndHoldAction = settings->value("timeIndicatorPressAndHoldAction", QVariant::fromValue(TIIB_ShowGoTo)).value<TimeIndicatorInteractionBehavior>();
        emit timeIndicatorPressAndHoldActionChanged();
        d->timeIndicatorTextFineTuneEnabled = settings->value("timeIndicatorTextFineTuneEnabled", true).toBool();
        emit timeIndicatorTextFineTuneEnabledChanged();
        d->timeIndicatorShowSliderOnHover = settings->value("timeIndicatorShowSliderOnHover", true).toBool();
        emit timeIndicatorShowSliderOnHoverChanged();
        settings->endGroup();
    }
    void BehaviorPreference::save() const {
        Q_D(const BehaviorPreference);
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(settingCategoryC);
        settings->setValue("startupBehavior", static_cast<int>(d->startupBehavior));
        settings->setValue("useSystemLanguage", d->useSystemLanguage);
        settings->setValue("localeName", d->localeName);
        settings->setValue("hasNotificationSoundAlert", d->hasNotificationSoundAlert);
        settings->setValue("notificationAutoHideTimeout", d->notificationAutoHideTimeout);
        settings->setValue("commandPaletteHistoryCount", d->commandPaletteHistoryCount);
        settings->setValue("proxyOption", static_cast<int>(d->proxyOption));
        settings->setValue("proxyType", static_cast<int>(d->proxyType));
        settings->setValue("proxyHostname", d->proxyHostname);
        settings->setValue("proxyPort", d->proxyPort);
        settings->setValue("proxyHasAuthentication", d->proxyHasAuthentication);
        settings->setValue("proxyUsername", d->proxyUsername);
        settings->setValue("proxyPassword", d->proxyPassword);
        settings->setValue("autoCheckForUpdates", d->autoCheckForUpdates);
        settings->setValue("updateOption", static_cast<int>(d->updateOption));
        settings->setValue("useCustomFont", d->useCustomFont);
        settings->setValue("fontFamily", d->fontFamily);
        settings->setValue("fontStyle", d->fontStyle);
        settings->setValue("uiBehavior", static_cast<int>(d->uiBehavior));
        settings->setValue("graphicsBehavior", static_cast<int>(d->graphicsBehavior));
        settings->setValue("animationEnabled", d->animationEnabled);
        settings->setValue("animationSpeedRatio", d->animationSpeedRatio);
        settings->setValue("timeIndicatorBackgroundVisible", d->timeIndicatorBackgroundVisible);
        settings->setValue("timeIndicatorClickAction", static_cast<int>(d->timeIndicatorClickAction));
        settings->setValue("timeIndicatorDoubleClickAction", static_cast<int>(d->timeIndicatorDoubleClickAction));
        settings->setValue("timeIndicatorPressAndHoldAction", static_cast<int>(d->timeIndicatorPressAndHoldAction));
        settings->setValue("timeIndicatorTextFineTuneEnabled", d->timeIndicatorTextFineTuneEnabled);
        settings->setValue("timeIndicatorShowSliderOnHover", d->timeIndicatorShowSliderOnHover);
        settings->endGroup();
    }

#define M_INSTANCE_D Q_ASSERT(m_instance);auto d = m_instance->d_func()
    
    BehaviorPreference::StartupBehavior BehaviorPreference::startupBehavior() {
        M_INSTANCE_D;
        return d->startupBehavior;
    }
    void BehaviorPreference::setStartupBehavior(StartupBehavior startupBehavior) {
        M_INSTANCE_D;
        if (d->startupBehavior == startupBehavior)
            return;
        d->startupBehavior = startupBehavior;
        emit m_instance->startupBehaviorChanged();
    }
    bool BehaviorPreference::useSystemLanguage() {
        M_INSTANCE_D;
        return d->useSystemLanguage;
    }
    void BehaviorPreference::setUseSystemLanguage(bool useSystemLanguage) {
        M_INSTANCE_D;
        if (d->useSystemLanguage == useSystemLanguage)
            return;
        d->useSystemLanguage = useSystemLanguage;
        emit m_instance->useSystemLanguageChanged();
    }
    QString BehaviorPreference::localeName() {
        M_INSTANCE_D;
        return d->localeName;
    }
    void BehaviorPreference::setLocaleName(const QString &localeName) {
        M_INSTANCE_D;
        if (d->localeName == localeName)
            return;
        d->localeName = localeName;
        emit m_instance->localeNameChanged();
    }
    bool BehaviorPreference::hasNotificationSoundAlert() {
        M_INSTANCE_D;
        return d->hasNotificationSoundAlert;
    }
    void BehaviorPreference::setHasNotificationSoundAlert(bool hasNotificationSoundAlert) {
        M_INSTANCE_D;
        if (d->hasNotificationSoundAlert == hasNotificationSoundAlert)
            return;
        d->hasNotificationSoundAlert = hasNotificationSoundAlert;
        emit m_instance->hasNotificationSoundAlertChanged();
    }
    int BehaviorPreference::notificationAutoHideTimeout() {
        M_INSTANCE_D;
        return d->notificationAutoHideTimeout;
    }
    void BehaviorPreference::setNotificationAutoHideTimeout(int notificationAutoHideTimeout) {
        M_INSTANCE_D;
        if (d->notificationAutoHideTimeout == notificationAutoHideTimeout)
            return;
        d->notificationAutoHideTimeout = notificationAutoHideTimeout;
        emit m_instance->notificationAutoHideTimeoutChanged();
    }
    int BehaviorPreference::commandPaletteHistoryCount() {
        M_INSTANCE_D;
        return d->commandPaletteHistoryCount;
    }
    void BehaviorPreference::setCommandPaletteHistoryCount(int commandPaletteHistoryCount) {
        M_INSTANCE_D;
        if (d->commandPaletteHistoryCount == commandPaletteHistoryCount)
            return;
        d->commandPaletteHistoryCount = commandPaletteHistoryCount;
        emit m_instance->commandPaletteHistoryCountChanged();
    }
    BehaviorPreference::ProxyOption BehaviorPreference::proxyOption() {
        M_INSTANCE_D;
        return d->proxyOption;
    }
    void BehaviorPreference::setProxyOption(ProxyOption proxyOption) {
        M_INSTANCE_D;
        if (d->proxyOption == proxyOption)
            return;
        d->proxyOption = proxyOption;
        emit m_instance->proxyOptionChanged();
    }
    BehaviorPreference::ProxyType BehaviorPreference::proxyType() {
        M_INSTANCE_D;
        return d->proxyType;
    }
    void BehaviorPreference::setProxyType(ProxyType proxyType) {
        M_INSTANCE_D;
        if (d->proxyType == proxyType)
            return;
        d->proxyType = proxyType;
        emit m_instance->proxyTypeChanged();
    }
    QString BehaviorPreference::proxyHostname() {
        M_INSTANCE_D;
        return d->proxyHostname;
    }
    void BehaviorPreference::setProxyHostname(const QString &proxyHostname) {
        M_INSTANCE_D;
        if (d->proxyHostname == proxyHostname)
            return;
        d->proxyHostname = proxyHostname;
        emit m_instance->proxyHostnameChanged();
    }
    quint16 BehaviorPreference::proxyPort() {
        M_INSTANCE_D;
        return d->proxyPort;
    }
    void BehaviorPreference::setProxyPort(quint16 proxyPort) {
        M_INSTANCE_D;
        if (d->proxyPort == proxyPort)
            return;
        d->proxyPort = proxyPort;
        emit m_instance->proxyPortChanged();
    }

    bool BehaviorPreference::proxyHasAuthentication() {
        M_INSTANCE_D;
        return d->proxyHasAuthentication;
    }

    void BehaviorPreference::setProxyHasAuthentication(bool proxyHasAuthentication) {
        M_INSTANCE_D;
        if (d->proxyHasAuthentication == proxyHasAuthentication)
            return;
        d->proxyHasAuthentication = proxyHasAuthentication;
        emit m_instance->proxyHasAuthenticationChanged();
    }
    QString BehaviorPreference::proxyUsername() {
        M_INSTANCE_D;
        return d->proxyUsername;
    }
    void BehaviorPreference::setProxyUsername(const QString &proxyUsername) {
        M_INSTANCE_D;
        if (d->proxyUsername == proxyUsername)
            return;
        d->proxyUsername = proxyUsername;
        emit m_instance->proxyUsernameChanged();
    }
    QString BehaviorPreference::proxyPassword() {
        M_INSTANCE_D;
        return d->proxyPassword;
    }
    void BehaviorPreference::setProxyPassword(const QString &proxyPassword) {
        M_INSTANCE_D;
        if (d->proxyPassword == proxyPassword)
            return;
        d->proxyPassword = proxyPassword;
        emit m_instance->proxyPasswordChanged();
    }
    bool BehaviorPreference::autoCheckForUpdates() {
        M_INSTANCE_D;
        return d->autoCheckForUpdates;
    }
    void BehaviorPreference::setAutoCheckForUpdates(bool autoCheckForUpdates) {
        M_INSTANCE_D;
        if (d->autoCheckForUpdates == autoCheckForUpdates)
            return;
        d->autoCheckForUpdates = autoCheckForUpdates;
        emit m_instance->autoCheckForUpdatesChanged();
    }
    BehaviorPreference::UpdateOption BehaviorPreference::updateOption() {
        M_INSTANCE_D;
        return d->updateOption;
    }
    void BehaviorPreference::setUpdateOption(UpdateOption updateOption) {
        M_INSTANCE_D;
        if (d->updateOption == updateOption)
            return;
        d->updateOption = updateOption;
        emit m_instance->updateOptionChanged();
    }
    bool BehaviorPreference::useCustomFont() {
        M_INSTANCE_D;
        return d->useCustomFont;
    }
    void BehaviorPreference::setUseCustomFont(bool useCustomFont) {
        M_INSTANCE_D;
        if (d->useCustomFont == useCustomFont)
            return;
        d->useCustomFont = useCustomFont;
        emit m_instance->useCustomFontChanged();
    }
    QString BehaviorPreference::fontFamily() {
        M_INSTANCE_D;
        return d->fontFamily;
    }
    void BehaviorPreference::setFontFamily(const QString &fontFamily) {
        M_INSTANCE_D;
        if (d->fontFamily == fontFamily)
            return;
        d->fontFamily = fontFamily;
        emit m_instance->fontFamilyChanged();
    }

    QString BehaviorPreference::fontStyle() {
        M_INSTANCE_D;
        return d->fontStyle;
    }

    void BehaviorPreference::setFontStyle(const QString &fontStyle) {
        M_INSTANCE_D;
        if (d->fontStyle == fontStyle)
            return;
        d->fontStyle = fontStyle;
        emit m_instance->fontStyleChanged();
    }

    BehaviorPreference::UIBehavior BehaviorPreference::uiBehavior() {
        M_INSTANCE_D;
        return d->uiBehavior;
    }
    void BehaviorPreference::setUiBehavior(UIBehavior uiBehavior) {
        M_INSTANCE_D;
        if (d->uiBehavior == uiBehavior)
            return;
        d->uiBehavior = uiBehavior;
        emit m_instance->uiBehaviorChanged();
    }
    BehaviorPreference::GraphicsBehavior BehaviorPreference::graphicsBehavior() {
        M_INSTANCE_D;
        return d->graphicsBehavior;
    }
    void BehaviorPreference::setGraphicsBehavior(GraphicsBehavior graphicsBehavior) {
        M_INSTANCE_D;
        if (d->graphicsBehavior == graphicsBehavior)
            return;
        d->graphicsBehavior = graphicsBehavior;
        emit m_instance->graphicsBehaviorChanged();
    }
    bool BehaviorPreference::isAnimationEnabled() {
        M_INSTANCE_D;
        return d->animationEnabled;
    }
    void BehaviorPreference::setAnimationEnabled(bool animationEnabled) {
        M_INSTANCE_D;
        if (d->animationEnabled == animationEnabled)
            return;
        d->animationEnabled = animationEnabled;
        emit m_instance->animationEnabledChanged();
    }
    double BehaviorPreference::animationSpeedRatio() {
        M_INSTANCE_D;
        return d->animationSpeedRatio;
    }
    void BehaviorPreference::setAnimationSpeedRatio(double animationSpeedRatio) {
        M_INSTANCE_D;
        if (d->animationSpeedRatio == animationSpeedRatio)
            return;
        d->animationSpeedRatio = animationSpeedRatio;
        emit m_instance->animationSpeedRatioChanged();
    }
    bool BehaviorPreference::timeIndicatorBackgroundVisible() {
        M_INSTANCE_D;
        return d->timeIndicatorBackgroundVisible;
    }
    void BehaviorPreference::setTimeIndicatorBackgroundVisible(bool timeIndicatorBackgroundVisible) {
        M_INSTANCE_D;
        if (d->timeIndicatorBackgroundVisible == timeIndicatorBackgroundVisible)
            return;
        d->timeIndicatorBackgroundVisible = timeIndicatorBackgroundVisible;
        emit m_instance->timeIndicatorBackgroundVisibleChanged();
    }
    BehaviorPreference::TimeIndicatorInteractionBehavior BehaviorPreference::timeIndicatorClickAction() {
        M_INSTANCE_D;
        return d->timeIndicatorClickAction;
    }
    void BehaviorPreference::setTimeIndicatorClickAction(TimeIndicatorInteractionBehavior timeIndicatorClickAction) {
        M_INSTANCE_D;
        if (d->timeIndicatorClickAction == timeIndicatorClickAction)
            return;
        d->timeIndicatorClickAction = timeIndicatorClickAction;
        emit m_instance->timeIndicatorClickActionChanged();
    }
    BehaviorPreference::TimeIndicatorInteractionBehavior BehaviorPreference::timeIndicatorDoubleClickAction() {
        M_INSTANCE_D;
        return d->timeIndicatorDoubleClickAction;
    }
    void BehaviorPreference::setTimeIndicatorDoubleClickAction(TimeIndicatorInteractionBehavior timeIndicatorDoubleClickAction) {
        M_INSTANCE_D;
        if (d->timeIndicatorDoubleClickAction == timeIndicatorDoubleClickAction)
            return;
        d->timeIndicatorDoubleClickAction = timeIndicatorDoubleClickAction;
        emit m_instance->timeIndicatorDoubleClickActionChanged();
    }
    BehaviorPreference::TimeIndicatorInteractionBehavior BehaviorPreference::timeIndicatorPressAndHoldAction() {
        M_INSTANCE_D;
        return d->timeIndicatorPressAndHoldAction;
    }
    void BehaviorPreference::setTimeIndicatorPressAndHoldAction(TimeIndicatorInteractionBehavior timeIndicatorPressAndHoldAction) {
        M_INSTANCE_D;
        if (d->timeIndicatorPressAndHoldAction == timeIndicatorPressAndHoldAction)
            return;
        d->timeIndicatorPressAndHoldAction = timeIndicatorPressAndHoldAction;
        emit m_instance->timeIndicatorPressAndHoldActionChanged();
    }
    bool BehaviorPreference::timeIndicatorTextFineTuneEnabled() {
        M_INSTANCE_D;
        return d->timeIndicatorTextFineTuneEnabled;
    }
    void BehaviorPreference::setTimeIndicatorTextFineTuneEnabled(bool timeIndicatorTextFineTuneEnabled) {
        M_INSTANCE_D;
        if (d->timeIndicatorTextFineTuneEnabled == timeIndicatorTextFineTuneEnabled)
            return;
        d->timeIndicatorTextFineTuneEnabled = timeIndicatorTextFineTuneEnabled;
        emit m_instance->timeIndicatorTextFineTuneEnabledChanged();
    }
    bool BehaviorPreference::timeIndicatorShowSliderOnHover() {
        M_INSTANCE_D;
        return d->timeIndicatorShowSliderOnHover;
    }
    void BehaviorPreference::setTimeIndicatorShowSliderOnHover(bool timeIndicatorShowSliderOnHover) {
        M_INSTANCE_D;
        if (d->timeIndicatorShowSliderOnHover == timeIndicatorShowSliderOnHover)
            return;
        d->timeIndicatorShowSliderOnHover = timeIndicatorShowSliderOnHover;
        emit m_instance->timeIndicatorShowSliderOnHoverChanged();
    }
}
