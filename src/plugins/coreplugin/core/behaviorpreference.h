#ifndef DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H
#define DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H

#include <QObject>
#include <qqmlintegration.h>

namespace Core {

    class BehaviorPreferencePrivate;

    class BehaviorPreference : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(BehaviorPreference)
        Q_PROPERTY(BehaviorPreference::StartupBehavior startupBehavior READ startupBehavior WRITE setStartupBehavior NOTIFY startupBehaviorChanged)
        Q_PROPERTY(bool useSystemLanguage READ useSystemLanguage WRITE setUseSystemLanguage NOTIFY useSystemLanguageChanged)
        Q_PROPERTY(QString localeName READ localeName WRITE setLocaleName NOTIFY localeNameChanged)
        Q_PROPERTY(bool hasNotificationSoundAlert READ hasNotificationSoundAlert WRITE setHasNotificationSoundAlert NOTIFY hasNotificationSoundAlertChanged)
        Q_PROPERTY(int notificationAutoHideTimeout READ notificationAutoHideTimeout WRITE setNotificationAutoHideTimeout NOTIFY notificationAutoHideTimeoutChanged)
        Q_PROPERTY(BehaviorPreference::ProxyOption proxyOption READ proxyOption WRITE setProxyOption NOTIFY proxyOptionChanged)
        Q_PROPERTY(BehaviorPreference::ProxyType proxyType READ proxyType WRITE setProxyType NOTIFY proxyTypeChanged)
        Q_PROPERTY(QString proxyHostname READ proxyHostname WRITE setProxyHostname NOTIFY proxyHostnameChanged)
        Q_PROPERTY(quint16 proxyPort READ proxyPort WRITE setProxyPort NOTIFY proxyPortChanged)
        Q_PROPERTY(bool proxyHasAuthentication READ proxyHasAuthentication WRITE setProxyHasAuthentication NOTIFY proxyHasAuthenticationChanged)
        Q_PROPERTY(QString proxyUsername READ proxyUsername WRITE setProxyUsername NOTIFY proxyUsernameChanged)
        Q_PROPERTY(QString proxyPassword READ proxyPassword WRITE setProxyPassword NOTIFY proxyPasswordChanged)
        Q_PROPERTY(bool autoCheckForUpdates READ autoCheckForUpdates WRITE setAutoCheckForUpdates NOTIFY autoCheckForUpdatesChanged)
        Q_PROPERTY(BehaviorPreference::UpdateOption updateOption READ updateOption WRITE setUpdateOption NOTIFY updateOptionChanged)
        Q_PROPERTY(bool useCustomFont READ useCustomFont WRITE setUseCustomFont NOTIFY useCustomFontChanged)
        Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
        Q_PROPERTY(BehaviorPreference::UIBehavior uiBehavior READ uiBehavior WRITE setUiBehavior NOTIFY uiBehaviorChanged)
        Q_PROPERTY(BehaviorPreference::GraphicsBehavior graphicsBehavior READ graphicsBehavior WRITE setGraphicsBehavior NOTIFY graphicsBehaviorChanged)
        Q_PROPERTY(bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled NOTIFY animationEnabledChanged)
        Q_PROPERTY(double animationSpeedRatio READ animationSpeedRatio WRITE setAnimationSpeedRatio NOTIFY animationSpeedRatioChanged)

    public:
        explicit BehaviorPreference(QObject *parent = nullptr);
        ~BehaviorPreference() override;

        void load();
        void save() const;

        enum StartupBehaviorFlag {
            SB_CreateNewProject = 0x01,
            SB_AutoOpenPreviousProjects = 0x02,
            SB_CloseHomeWindowAfterOpeningProject = 0x04,
        };
        Q_ENUM(StartupBehaviorFlag)
        Q_DECLARE_FLAGS(StartupBehavior, StartupBehaviorFlag)

        StartupBehavior startupBehavior() const;
        void setStartupBehavior(StartupBehavior startupBehavior);

        bool useSystemLanguage() const;
        void setUseSystemLanguage(bool useSystemLanguage);

        QString localeName() const;
        void setLocaleName(const QString &localeName);

        bool hasNotificationSoundAlert() const;
        void setHasNotificationSoundAlert(bool hasNotificationSoundAlert);

        int notificationAutoHideTimeout() const;
        void setNotificationAutoHideTimeout(int notificationAutoHideTimeout);

        enum ProxyOption {
            PO_None,
            PO_System,
            PO_Manual,
        };
        Q_ENUM(ProxyOption)

        ProxyOption proxyOption() const;
        void setProxyOption(ProxyOption proxyOption);

        enum ProxyType {
            PT_Socks5,
            PT_Http,
        };
        Q_ENUM(ProxyType)

        ProxyType proxyType() const;
        void setProxyType(ProxyType proxyType);

        QString proxyHostname() const;
        void setProxyHostname(const QString &proxyHostname);

        quint16 proxyPort() const;
        void setProxyPort(quint16 proxyPort);

        bool proxyHasAuthentication() const;
        void setProxyHasAuthentication(bool proxyHasAuthentication);

        QString proxyUsername() const;
        void setProxyUsername(const QString &proxyUsername);

        QString proxyPassword() const;
        void setProxyPassword(const QString &proxyPassword);

        bool autoCheckForUpdates() const;
        void setAutoCheckForUpdates(bool autoCheckForUpdates);

        enum UpdateOption {
            UO_Stable,
            UO_Beta,
        };
        Q_ENUM(UpdateOption)

        UpdateOption updateOption() const;
        void setUpdateOption(UpdateOption updateOption);

        bool useCustomFont() const;
        void setUseCustomFont(bool useCustomFont);

        QString fontFamily() const;
        void setFontFamily(const QString &fontFamily);

        enum UIBehaviorFlag {
            UB_Frameless = 0x01,
            UB_MergeMenuAndTitleBar = 0x02,
            UB_NativeMenu = 0x04,
            UB_FullPath = 0x08,
        };
        Q_ENUM(UIBehaviorFlag)
        Q_DECLARE_FLAGS(UIBehavior, UIBehaviorFlag)

        UIBehavior uiBehavior() const;
        void setUiBehavior(UIBehavior uiBehavior);

        enum GraphicsBehaviorFlag {
            GB_Hardware = 0x01,
            GB_Antialiasing = 0x02,
        };
        Q_ENUM(GraphicsBehaviorFlag)
        Q_DECLARE_FLAGS(GraphicsBehavior, GraphicsBehaviorFlag)

        GraphicsBehavior graphicsBehavior() const;
        void setGraphicsBehavior(GraphicsBehavior graphicsBehavior);

        bool isAnimationEnabled() const;
        void setAnimationEnabled(bool animationEnabled);

        double animationSpeedRatio() const;
        void setAnimationSpeedRatio(double animationSpeedRatio);

    Q_SIGNALS:
        void startupBehaviorChanged();
        void useSystemLanguageChanged();
        void localeNameChanged();
        void hasNotificationSoundAlertChanged();
        void notificationAutoHideTimeoutChanged();
        void proxyOptionChanged();
        void proxyTypeChanged();
        void proxyHostnameChanged();
        void proxyPortChanged();
        void proxyHasAuthenticationChanged();
        void proxyUsernameChanged();
        void proxyPasswordChanged();
        void autoCheckForUpdatesChanged();
        void updateOptionChanged();
        void useCustomFontChanged();
        void fontFamilyChanged();
        void uiBehaviorChanged();
        void graphicsBehaviorChanged();
        void animationEnabledChanged();
        void animationSpeedRatioChanged();

    private:
        QScopedPointer<BehaviorPreferencePrivate> d_ptr;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::BehaviorPreference::StartupBehavior)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::BehaviorPreference::UIBehavior)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::BehaviorPreference::GraphicsBehavior)

#endif //DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H
