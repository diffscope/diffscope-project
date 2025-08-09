#include "behaviorpreference.h"

#include <QApplication>
#include <QSettings>

#include <CoreApi/plugindatabase.h>

namespace Core {

    class BehaviorPreferencePrivate {
    public:
        bool initialized{};

        BehaviorPreference::StartupBehavior startupBehavior{};
        bool useSystemLanguage{};
        QString localeName{};
        bool hasNotificationSoundAlert{};
        int notificationAutoHideTimeout{};
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
    };

    BehaviorPreference::BehaviorPreference(QObject *parent) : QObject(parent), d_ptr(new BehaviorPreferencePrivate) {
    }
    BehaviorPreference::~BehaviorPreference() = default;

    static constexpr char settingCategoryC[] = "Core::BehaviorPreference";

    void BehaviorPreference::load() {
        Q_D(BehaviorPreference);
        auto settings = PluginDatabase::settings();
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
        d->uiBehavior = settings->value("uiBehavior", QVariant::fromValue(UB_Frameless | UB_MergeMenuAndTitleBar | UB_NativeMenu)).value<UIBehavior>();
        emit uiBehaviorChanged();
        d->graphicsBehavior = settings->value("graphicsBehavior", QVariant::fromValue(GB_Hardware |GB_Antialiasing)).value<GraphicsBehavior>();
        emit graphicsBehaviorChanged();
        d->animationEnabled = settings->value("animationEnabled", true).toBool();
        emit animationEnabledChanged();
        d->animationSpeedRatio = settings->value("animationSpeedRatio", 1.0).toDouble();
        emit animationSpeedRatioChanged();
        settings->endGroup();
    }
    void BehaviorPreference::save() const {
        Q_D(const BehaviorPreference);
        auto settings = PluginDatabase::settings();
        settings->beginGroup(settingCategoryC);
        settings->setValue("startupBehavior", static_cast<int>(d->startupBehavior));
        settings->setValue("useSystemLanguage", d->useSystemLanguage);
        settings->setValue("localeName", d->localeName);
        settings->setValue("hasNotificationSoundAlert", d->hasNotificationSoundAlert);
        settings->setValue("notificationAutoHideTimeout", d->notificationAutoHideTimeout);
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
        settings->endGroup();
    }
    BehaviorPreference::StartupBehavior BehaviorPreference::startupBehavior() const {
        Q_D(const BehaviorPreference);
        return d->startupBehavior;
    }
    void BehaviorPreference::setStartupBehavior(StartupBehavior startupBehavior) {
        Q_D(BehaviorPreference);
        if (d->startupBehavior == startupBehavior)
            return;
        d->startupBehavior = startupBehavior;
        emit startupBehaviorChanged();
    }
    bool BehaviorPreference::useSystemLanguage() const {
        Q_D(const BehaviorPreference);
        return d->useSystemLanguage;
    }
    void BehaviorPreference::setUseSystemLanguage(bool useSystemLanguage) {
        Q_D(BehaviorPreference);
        if (d->useSystemLanguage == useSystemLanguage)
            return;
        d->useSystemLanguage = useSystemLanguage;
        emit useSystemLanguageChanged();
    }
    QString BehaviorPreference::localeName() const {
        Q_D(const BehaviorPreference);
        return d->localeName;
    }
    void BehaviorPreference::setLocaleName(const QString &localeName) {
        Q_D(BehaviorPreference);
        if (d->localeName == localeName)
            return;
        d->localeName = localeName;
        emit localeNameChanged();
    }
    bool BehaviorPreference::hasNotificationSoundAlert() const {
        Q_D(const BehaviorPreference);
        return d->hasNotificationSoundAlert;
    }
    void BehaviorPreference::setHasNotificationSoundAlert(bool hasNotificationSoundAlert) {
        Q_D(BehaviorPreference);
        if (d->hasNotificationSoundAlert == hasNotificationSoundAlert)
            return;
        d->hasNotificationSoundAlert = hasNotificationSoundAlert;
        emit hasNotificationSoundAlertChanged();
    }
    int BehaviorPreference::notificationAutoHideTimeout() const {
        Q_D(const BehaviorPreference);
        return d->notificationAutoHideTimeout;
    }
    void BehaviorPreference::setNotificationAutoHideTimeout(int notificationAutoHideTimeout) {
        Q_D(BehaviorPreference);
        if (d->notificationAutoHideTimeout == notificationAutoHideTimeout)
            return;
        d->notificationAutoHideTimeout = notificationAutoHideTimeout;
        emit notificationAutoHideTimeoutChanged();
    }
    BehaviorPreference::ProxyOption BehaviorPreference::proxyOption() const {
        Q_D(const BehaviorPreference);
        return d->proxyOption;
    }
    void BehaviorPreference::setProxyOption(ProxyOption proxyOption) {
        Q_D(BehaviorPreference);
        if (d->proxyOption == proxyOption)
            return;
        d->proxyOption = proxyOption;
        emit proxyOptionChanged();
    }
    BehaviorPreference::ProxyType BehaviorPreference::proxyType() const {
        Q_D(const BehaviorPreference);
        return d->proxyType;
    }
    void BehaviorPreference::setProxyType(ProxyType proxyType) {
        Q_D(BehaviorPreference);
        if (d->proxyType == proxyType)
            return;
        d->proxyType = proxyType;
        emit proxyTypeChanged();
    }
    QString BehaviorPreference::proxyHostname() const {
        Q_D(const BehaviorPreference);
        return d->proxyHostname;
    }
    void BehaviorPreference::setProxyHostname(const QString &proxyHostname) {
        Q_D(BehaviorPreference);
        if (d->proxyHostname == proxyHostname)
            return;
        d->proxyHostname = proxyHostname;
        emit proxyHostnameChanged();
    }
    quint16 BehaviorPreference::proxyPort() const {
        Q_D(const BehaviorPreference);
        return d->proxyPort;
    }
    void BehaviorPreference::setProxyPort(quint16 proxyPort) {
        Q_D(BehaviorPreference);
        if (d->proxyPort == proxyPort)
            return;
        d->proxyPort = proxyPort;
        emit proxyPortChanged();
    }

    bool BehaviorPreference::proxyHasAuthentication() const {
        Q_D(const BehaviorPreference);
        return d->proxyHasAuthentication;
    }

    void BehaviorPreference::setProxyHasAuthentication(bool proxyHasAuthentication) {
        Q_D(BehaviorPreference);
        if (d->proxyHasAuthentication == proxyHasAuthentication)
            return;
        d->proxyHasAuthentication = proxyHasAuthentication;
        emit proxyHasAuthenticationChanged();
    }
    QString BehaviorPreference::proxyUsername() const {
        Q_D(const BehaviorPreference);
        return d->proxyUsername;
    }
    void BehaviorPreference::setProxyUsername(const QString &proxyUsername) {
        Q_D(BehaviorPreference);
        if (d->proxyUsername == proxyUsername)
            return;
        d->proxyUsername = proxyUsername;
        emit proxyUsernameChanged();
    }
    QString BehaviorPreference::proxyPassword() const {
        Q_D(const BehaviorPreference);
        return d->proxyPassword;
    }
    void BehaviorPreference::setProxyPassword(const QString &proxyPassword) {
        Q_D(BehaviorPreference);
        if (d->proxyPassword == proxyPassword)
            return;
        d->proxyPassword = proxyPassword;
        emit proxyPasswordChanged();
    }
    bool BehaviorPreference::autoCheckForUpdates() const {
        Q_D(const BehaviorPreference);
        return d->autoCheckForUpdates;
    }
    void BehaviorPreference::setAutoCheckForUpdates(bool autoCheckForUpdates) {
        Q_D(BehaviorPreference);
        if (d->autoCheckForUpdates == autoCheckForUpdates)
            return;
        d->autoCheckForUpdates = autoCheckForUpdates;
        emit autoCheckForUpdatesChanged();
    }
    BehaviorPreference::UpdateOption BehaviorPreference::updateOption() const {
        Q_D(const BehaviorPreference);
        return d->updateOption;
    }
    void BehaviorPreference::setUpdateOption(UpdateOption updateOption) {
        Q_D(BehaviorPreference);
        if (d->updateOption == updateOption)
            return;
        d->updateOption = updateOption;
        emit updateOptionChanged();
    }
    bool BehaviorPreference::useCustomFont() const {
        Q_D(const BehaviorPreference);
        return d->useCustomFont;
    }
    void BehaviorPreference::setUseCustomFont(bool useCustomFont) {
        Q_D(BehaviorPreference);
        if (d->useCustomFont == useCustomFont)
            return;
        d->useCustomFont = useCustomFont;
        emit useCustomFontChanged();
    }
    QString BehaviorPreference::fontFamily() const {
        Q_D(const BehaviorPreference);
        return d->fontFamily;
    }
    void BehaviorPreference::setFontFamily(const QString &fontFamily) {
        Q_D(BehaviorPreference);
        if (d->fontFamily == fontFamily)
            return;
        d->fontFamily = fontFamily;
        emit fontFamilyChanged();
    }

    QString BehaviorPreference::fontStyle() const {
        Q_D(const BehaviorPreference);
        return d->fontStyle;
    }

    void BehaviorPreference::setFontStyle(const QString &fontStyle) {
        Q_D(BehaviorPreference);
        if (d->fontStyle == fontStyle)
            return;
        d->fontStyle = fontStyle;
        emit fontStyleChanged();
    }

    BehaviorPreference::UIBehavior BehaviorPreference::uiBehavior() const {
        Q_D(const BehaviorPreference);
        return d->uiBehavior;
    }
    void BehaviorPreference::setUiBehavior(UIBehavior uiBehavior) {
        Q_D(BehaviorPreference);
        if (d->uiBehavior == uiBehavior)
            return;
        d->uiBehavior = uiBehavior;
        emit uiBehaviorChanged();
    }
    BehaviorPreference::GraphicsBehavior BehaviorPreference::graphicsBehavior() const {
        Q_D(const BehaviorPreference);
        return d->graphicsBehavior;
    }
    void BehaviorPreference::setGraphicsBehavior(GraphicsBehavior graphicsBehavior) {
        Q_D(BehaviorPreference);
        if (d->graphicsBehavior == graphicsBehavior)
            return;
        d->graphicsBehavior = graphicsBehavior;
        emit graphicsBehaviorChanged();
    }
    bool BehaviorPreference::isAnimationEnabled() const {
        Q_D(const BehaviorPreference);
        return d->animationEnabled;
    }
    void BehaviorPreference::setAnimationEnabled(bool animationEnabled) {
        Q_D(BehaviorPreference);
        if (d->animationEnabled == animationEnabled)
            return;
        d->animationEnabled = animationEnabled;
        emit animationEnabledChanged();
    }
    double BehaviorPreference::animationSpeedRatio() const {
        Q_D(const BehaviorPreference);
        return d->animationSpeedRatio;
    }
    void BehaviorPreference::setAnimationSpeedRatio(double animationSpeedRatio) {
        Q_D(BehaviorPreference);
        if (d->animationSpeedRatio == animationSpeedRatio)
            return;
        d->animationSpeedRatio = animationSpeedRatio;
        emit animationSpeedRatioChanged();
    }
}