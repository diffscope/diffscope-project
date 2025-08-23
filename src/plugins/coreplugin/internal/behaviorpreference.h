#ifndef DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H
#define DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H

#include <QObject>
#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

namespace Core::Internal {

    class CorePlugin;

    class BehaviorPreferencePrivate;

    class BehaviorPreference : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(BehaviorPreference)
        Q_PROPERTY(BehaviorPreference::StartupBehavior startupBehavior READ startupBehavior WRITE setStartupBehavior NOTIFY startupBehaviorChanged)
        Q_PROPERTY(bool useSystemLanguage READ useSystemLanguage WRITE setUseSystemLanguage NOTIFY useSystemLanguageChanged)
        Q_PROPERTY(QString localeName READ localeName WRITE setLocaleName NOTIFY localeNameChanged)
        Q_PROPERTY(bool hasNotificationSoundAlert READ hasNotificationSoundAlert WRITE setHasNotificationSoundAlert NOTIFY hasNotificationSoundAlertChanged)
        Q_PROPERTY(int notificationAutoHideTimeout READ notificationAutoHideTimeout WRITE setNotificationAutoHideTimeout NOTIFY notificationAutoHideTimeoutChanged)
        Q_PROPERTY(int commandPaletteHistoryCount READ commandPaletteHistoryCount WRITE setCommandPaletteHistoryCount NOTIFY commandPaletteHistoryCountChanged)
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
        Q_PROPERTY(QString fontStyle READ fontStyle WRITE setFontStyle NOTIFY fontStyleChanged)
        Q_PROPERTY(BehaviorPreference::UIBehavior uiBehavior READ uiBehavior WRITE setUiBehavior NOTIFY uiBehaviorChanged)
        Q_PROPERTY(BehaviorPreference::GraphicsBehavior graphicsBehavior READ graphicsBehavior WRITE setGraphicsBehavior NOTIFY graphicsBehaviorChanged)
        Q_PROPERTY(bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled NOTIFY animationEnabledChanged)
        Q_PROPERTY(double animationSpeedRatio READ animationSpeedRatio WRITE setAnimationSpeedRatio NOTIFY animationSpeedRatioChanged)

    public:
        ~BehaviorPreference() override;

        static BehaviorPreference *instance();

        static inline BehaviorPreference *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        void load();
        void save() const;

        enum StartupBehaviorFlag {
            SB_CreateNewProject = 0x01,
            SB_AutoOpenPreviousProjects = 0x02,
            SB_CloseHomeWindowAfterOpeningProject = 0x04,
        };
        Q_ENUM(StartupBehaviorFlag)
        Q_DECLARE_FLAGS(StartupBehavior, StartupBehaviorFlag)

        static StartupBehavior startupBehavior();
        static void setStartupBehavior(StartupBehavior startupBehavior);

        static bool useSystemLanguage();
        static void setUseSystemLanguage(bool useSystemLanguage);

        static QString localeName();
        static void setLocaleName(const QString &localeName);

        static bool hasNotificationSoundAlert();
        static void setHasNotificationSoundAlert(bool hasNotificationSoundAlert);

        static int notificationAutoHideTimeout();
        static void setNotificationAutoHideTimeout(int notificationAutoHideTimeout);

        static int commandPaletteHistoryCount();
        static void setCommandPaletteHistoryCount(int commandPaletteHistoryCount);

        enum ProxyOption {
            PO_None,
            PO_System,
            PO_Manual,
        };
        Q_ENUM(ProxyOption)

        static ProxyOption proxyOption();
        static void setProxyOption(ProxyOption proxyOption);

        enum ProxyType {
            PT_Socks5,
            PT_Http,
        };
        Q_ENUM(ProxyType)

        static ProxyType proxyType();
        static void setProxyType(ProxyType proxyType);

        static QString proxyHostname();
        static void setProxyHostname(const QString &proxyHostname);

        static quint16 proxyPort();
        static void setProxyPort(quint16 proxyPort);

        static bool proxyHasAuthentication();
        static void setProxyHasAuthentication(bool proxyHasAuthentication);

        static QString proxyUsername();
        static void setProxyUsername(const QString &proxyUsername);

        static QString proxyPassword();
        static void setProxyPassword(const QString &proxyPassword);

        static bool autoCheckForUpdates();
        static void setAutoCheckForUpdates(bool autoCheckForUpdates);

        enum UpdateOption {
            UO_Stable,
            UO_Beta,
        };
        Q_ENUM(UpdateOption)

        static UpdateOption updateOption();
        static void setUpdateOption(UpdateOption updateOption);

        static bool useCustomFont();
        static void setUseCustomFont(bool useCustomFont);

        static QString fontFamily();
        static void setFontFamily(const QString &fontFamily);

        static QString fontStyle();
        static void setFontStyle(const QString &fontStyle);

        enum UIBehaviorFlag {
            UB_Frameless = 0x01,
            UB_MergeMenuAndTitleBar = 0x02,
            UB_NativeMenu = 0x04,
            UB_FullPath = 0x08,
        };
        Q_ENUM(UIBehaviorFlag)
        Q_DECLARE_FLAGS(UIBehavior, UIBehaviorFlag)

        static UIBehavior uiBehavior();
        static void setUiBehavior(UIBehavior uiBehavior);

        enum GraphicsBehaviorFlag {
            GB_Hardware = 0x01,
            GB_Antialiasing = 0x02,
        };
        Q_ENUM(GraphicsBehaviorFlag)
        Q_DECLARE_FLAGS(GraphicsBehavior, GraphicsBehaviorFlag)

        static GraphicsBehavior graphicsBehavior();
        static void setGraphicsBehavior(GraphicsBehavior graphicsBehavior);

        static bool isAnimationEnabled();
        static void setAnimationEnabled(bool animationEnabled);

        static double animationSpeedRatio();
        static void setAnimationSpeedRatio(double animationSpeedRatio);

    Q_SIGNALS:
        void startupBehaviorChanged();
        void useSystemLanguageChanged();
        void localeNameChanged();
        void hasNotificationSoundAlertChanged();
        void notificationAutoHideTimeoutChanged();
        void commandPaletteHistoryCountChanged();
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
        void fontStyleChanged();
        void uiBehaviorChanged();
        void graphicsBehaviorChanged();
        void animationEnabledChanged();
        void animationSpeedRatioChanged();

    private:
        friend class CorePlugin;
        explicit BehaviorPreference(QObject *parent = nullptr);
        QScopedPointer<BehaviorPreferencePrivate> d_ptr;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Internal::BehaviorPreference::StartupBehavior)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Internal::BehaviorPreference::UIBehavior)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Internal::BehaviorPreference::GraphicsBehavior)

#endif //DIFFSCOPE_COREPLUGIN_BEHAVIORPREFERENCE_H
