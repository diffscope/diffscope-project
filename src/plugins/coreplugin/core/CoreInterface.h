// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COREINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_COREINTERFACE_H

#include <QObject>
#include <qqmlintegration.h>

#include <CoreApi/coreinterfacebase.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/coreglobal.h>

class QQmlEngine;
class QJSEngine;
class QWindow;

namespace DspxProject {
    class CheckerRegistry;
    class ProjectLifecycleScenario;
}

namespace QAK {
    class ActionRegistry;
    class QuickActionContext;
}

namespace SingerCatalog {
    class DefaultLyricManager;
    class SingerRegistry;
}

namespace Notifications {
    class NotificationMessage;
    class NotificationModel;
}

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class FileOpenDispatcher;
    class PropertyEditorManager;
    class TrackColorSchema;
    class MixedSingerPresets;

    class CoreInterfacePrivate;

    class CORE_EXPORT CoreInterface : public CoreInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(CoreInterface)
        Q_PROPERTY(QAK::ActionRegistry *actionRegistry READ actionRegistry CONSTANT)
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_PROPERTY(TrackColorSchema *trackColorSchema READ trackColorSchema CONSTANT)
        Q_PROPERTY(SingerCatalog::DefaultLyricManager *defaultLyricManager READ defaultLyricManager CONSTANT)
        Q_PROPERTY(SingerCatalog::SingerRegistry *singerRegistry READ singerRegistry CONSTANT)
        Q_PROPERTY(MixedSingerPresets *mixedSingerPresets READ mixedSingerPresets CONSTANT)
        Q_PROPERTY(FileOpenDispatcher *fileOpenDispatcher READ fileOpenDispatcher CONSTANT)
        Q_PROPERTY(DspxProject::ProjectLifecycleScenario *projectLifecycleScenario READ projectLifecycleScenario CONSTANT)
        Q_PROPERTY(Notifications::NotificationModel *globalNotifications READ globalNotifications CONSTANT)
        Q_PROPERTY(Notifications::NotificationModel *allNotifications READ allNotifications CONSTANT)
    public:
        static CoreInterface *instance();
        static inline CoreInterface *create(QQmlEngine *, QJSEngine *) { return instance(); }

        static QAK::ActionRegistry *actionRegistry();
        static QAK::QuickActionContext *actionContext();

        static DspxProject::CheckerRegistry *dspxCheckerRegistry();

        static TrackColorSchema *trackColorSchema();

        static SingerCatalog::DefaultLyricManager *defaultLyricManager();

        static SingerCatalog::SingerRegistry *singerRegistry();

        static MixedSingerPresets *mixedSingerPresets();

        static FileOpenDispatcher *fileOpenDispatcher();

        static DspxProject::ProjectLifecycleScenario *projectLifecycleScenario();

        static Notifications::NotificationModel *globalNotifications();
        // Returns active notifications across all scopes.
        static Notifications::NotificationModel *allNotifications();

        static constexpr const char *dspxEditorId() {
            return "org.diffscope.diffscope";
        }

        enum NotificationBubbleMode {
            NormalBubble,
            DoNotShowBubble,
            AutoHide,
        };
        Q_ENUM(NotificationBubbleMode)

        Q_INVOKABLE static void sendNotification(Notifications::NotificationMessage *message, NotificationBubbleMode mode = NormalBubble);
        Q_INVOKABLE static void sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode = NormalBubble);

        Q_INVOKABLE static int execSettingsDialog(const QString &id, QWindow *parent);
        Q_INVOKABLE static void execPluginsDialog(QWindow *parent);
        Q_INVOKABLE static void execNotificationListDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutAppDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutQtDialog(QWindow *parent);
        Q_INVOKABLE static void showHome();

    Q_SIGNALS:
        void resetAllDoNotShowAgainRequested();

    private:
        explicit CoreInterface(QObject *parent = nullptr);
        ~CoreInterface();

        CoreInterface(CoreInterfacePrivate &d, QObject *parent = nullptr);

        friend class Internal::CorePlugin;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREINTERFACE_H
