// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "CoreInterface.h"

#include <application_buildinfo.h>
#include <application_config.h>

#include <csignal>
#include <memory>

#include <QApplication>
#include <QChildEvent>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QSettings>
#include <QJsonArray>
#include <QJsonObject>

#include <QtQuickTemplates2/private/qquickicon_p.h>

#include <CoreApi/private/coreinterfacebase_p.h>
#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <extensionsystem/pluginmanager.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/Theme.h>

#include <dspxproject/CheckerRegistry.h>
#include <dspxproject/ProjectLifecycleScenario.h>

#include <coreplugin/DefaultLyricManager.h>
#include <coreplugin/FileOpenDispatcher.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/MixedSingerPresets.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/TrackColorSchema.h>
#include <coreplugin/internal/CoreProjectLifecycleStrategy.h>
#include <coreplugin/internal/NotificationCenter.h>
#include <coreplugin/internal/NotificationManager.h>
#include <singercatalog/SingerRegistry.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcCoreInterface, "diffscope.core.coreinterface")

    class CoreInterfacePrivate : CoreInterfaceBasePrivate {
        Q_DECLARE_PUBLIC(CoreInterface)
    public:
        CoreInterfacePrivate() {
        }

        QQmlEngine *qmlEngine;
        QAK::ActionRegistry *actionRegistry;
        QAK::QuickActionContext *actionContext = nullptr;
        DspxProject::CheckerRegistry *dspxCheckerRegistry;
        TrackColorSchema *trackColorSchema;
        DefaultLyricManager *defaultLyricManager;
        SingerCatalog::SingerRegistry *singerRegistry;
        MixedSingerPresets *mixedSingerPresets;
        FileOpenDispatcher *fileOpenDispatcher;
        DspxProject::ProjectLifecycleScenario *projectLifecycleScenario;
        Internal::NotificationCenter *notificationCenter;

        void init() {
            Q_Q(CoreInterface);
            qmlEngine = new QQmlEngine(q);
            actionRegistry = new QAK::ActionRegistry(q);
            actionContext = new QAK::QuickActionContext(q);
            actionRegistry->addContext(actionContext);
            dspxCheckerRegistry = new DspxProject::CheckerRegistry(q);
            trackColorSchema = new TrackColorSchema(q);
            defaultLyricManager = new DefaultLyricManager(q);
            singerRegistry = new SingerCatalog::SingerRegistry(q);
            mixedSingerPresets = new MixedSingerPresets(q);
            fileOpenDispatcher = new FileOpenDispatcher(q);
            auto strategy = new Internal::CoreProjectLifecycleStrategy(dspxCheckerRegistry);
            projectLifecycleScenario = new DspxProject::ProjectLifecycleScenario(strategy, q);
            strategy->setParent(projectLifecycleScenario);
            notificationCenter = new Internal::NotificationCenter(q);
            QObject::connect(q, &CoreInterface::resetAllDoNotShowAgainRequested,
                             notificationCenter, &Internal::NotificationCenter::clearHiddenMessageIdentifiers);
        }
    };

    class OpenUrlHelper : public QObject {
        Q_OBJECT
    public Q_SLOTS:
        Q_INVOKABLE static inline void openUrl(const QString &url) {
            QDesktopServices::openUrl(QUrl(url));
        }
    };

    CoreInterface *CoreInterface::instance() {
        return static_cast<CoreInterface *>(CoreInterfaceBase::instance());
    }
    QAK::ActionRegistry *CoreInterface::actionRegistry() {
        Q_ASSERT(instance());
        return instance()->d_func()->actionRegistry;
    }

    QAK::QuickActionContext *CoreInterface::actionContext() {
        Q_ASSERT(instance());
        return instance()->d_func()->actionContext;
    }

    DspxProject::CheckerRegistry *CoreInterface::dspxCheckerRegistry() {
        Q_ASSERT(instance());
        return instance()->d_func()->dspxCheckerRegistry;
    }

    TrackColorSchema *CoreInterface::trackColorSchema() {
        Q_ASSERT(instance());
        return instance()->d_func()->trackColorSchema;
    }

    DefaultLyricManager *CoreInterface::defaultLyricManager() {
        Q_ASSERT(instance());
        return instance()->d_func()->defaultLyricManager;
    }

    SingerCatalog::SingerRegistry *CoreInterface::singerRegistry() {
        Q_ASSERT(instance());
        return instance()->d_func()->singerRegistry;
    }

    MixedSingerPresets *CoreInterface::mixedSingerPresets() {
        Q_ASSERT(instance());
        return instance()->d_func()->mixedSingerPresets;
    }

    FileOpenDispatcher *CoreInterface::fileOpenDispatcher() {
        Q_ASSERT(instance());
        return instance()->d_func()->fileOpenDispatcher;
    }

    DspxProject::ProjectLifecycleScenario *CoreInterface::projectLifecycleScenario() {
        Q_ASSERT(instance());
        return instance()->d_func()->projectLifecycleScenario;
    }

    static Internal::NotificationManager::NotificationBubbleMode toInternalNotificationBubbleMode(CoreInterface::NotificationBubbleMode mode) {
        return static_cast<Internal::NotificationManager::NotificationBubbleMode>(mode);
    }

    void CoreInterface::sendNotification(NotificationMessage *message, NotificationBubbleMode mode) {
        Q_ASSERT(instance());
        Q_ASSERT(message);
        instance()->d_func()->notificationCenter->globalNotificationManager()->addMessage(message, toInternalNotificationBubbleMode(mode));
    }

    void CoreInterface::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode) {
        auto message = new NotificationMessage(instance());
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }

    int CoreInterface::execSettingsDialog(const QString &id, QWindow *parent) {
        static std::unique_ptr<QWindow> dlg;

        qCInfo(lcCoreInterface) << "Opening settings dialog" << id;

        // TODO: show last used page if id is empty

        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.settingDialog"));
        state->setValue(QStringLiteral("currentId"), state->value(QStringLiteral("currentId"), QStringLiteral("org.diffscope.core.General")));
        qCDebug(lcCoreInterface) << "Saved current id" << state->value(QStringLiteral("currentId")).toString();
        state->endGroup();

        if (dlg) {
            qCDebug(lcCoreInterface) << "Reusing existing settings dialog";
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            return -1;
        }

        int code;
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "SettingDialog");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto object = component.create();
            if (!object) {
                qFatal() << component.errorString();
            }
            auto window = qobject_cast<QWindow *>(object);
            if (!window) {
                qFatal("SettingDialog in DiffScope.Core is not QWindow");
            }
            dlg.reset(window);
            dlg->setTransientParent(parent);
            qCDebug(lcCoreInterface) << "Showing settings dialog";
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            dlg->show();
            QEventLoop eventLoop;
            connect(dlg.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            qCDebug(lcCoreInterface) << "Settings dialog finished";
            dlg.reset();
        }

        // return code;
        return 0;
    }

    void CoreInterface::execPluginsDialog(QWindow *parent) {
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PluginDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QObject> object(component.create());
        if (!object) {
            qFatal() << component.errorString();
        }
        auto dlg = qobject_cast<QWindow *>(object.get());
        if (!dlg) {
            qFatal("PluginDialog in DiffScope.Core is not QWindow");
        }
        dlg->setTransientParent(parent);
        dlg->show();
        QEventLoop eventLoop;
        connect(dlg, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
    }

    void CoreInterface::execNotificationListDialog(QWindow *parent) {
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "NotificationListDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto notificationModel = instance()->d_func()->notificationCenter->globalNotificationManager();
        std::unique_ptr<QObject> object(component.createWithInitialProperties({
            {"notificationModel", QVariant::fromValue(notificationModel)},
        }));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto dlg = qobject_cast<QWindow *>(object.get());
        if (!dlg) {
            qFatal("NotificationListDialog in DiffScope.Core is not QWindow");
        }
        dlg->setTransientParent(parent);
        dlg->show();
        QEventLoop eventLoop;
        connect(dlg, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
    }
    void CoreInterface::execAboutAppDialog(QWindow *parent) {
        static const QString appName = qApp->applicationDisplayName();

        QString aboutInfo =
            QApplication::translate(
                "Application",
                "<p>A professional singing-voice-synthesis editor powered by DiffSinger</p>"
                "<p>Version %1</p>"
                "<p>Copyright \u00a9 %2-%3 %4. All rights reserved.</p>"
                "<p>Visit <a href=\"%5\">%5</a> for more information.</p>"
            )
                .arg(
                    QApplication::applicationVersion(),
                    QLocale().toString(QDate(QStringLiteral(APPLICATION_DEV_START_YEAR).toInt(), 1, 1), "yyyy"),
                    QLocale().toString(QDate(QStringLiteral(APPLICATION_BUILD_YEAR).toInt(), 1, 1), "yyyy"),
                    APPLICATION_VENDOR_NAME,
                    QStringLiteral(APPLICATION_URL)
                );

        QString licenseInfo =
            QApplication::translate(
                "Application",
                "<h3>License</h3>"
                "<p>Licensed under the Apache License, Version 2.0.<br>"
                "You may obtain a copy of the License at %1.</p>"
                "<p>This application is distributed "
                "<b>AS IS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND</b>, either express or "
                "implied.</p>"
                "<p>This license does not apply to plugins. Please refer to Plugins to view the licenses applicable to each individual plugin.</p>"
            )
                .arg(QStringLiteral("<a "
                                    "href=\"https://www.apache.org/licenses/"
                                    "LICENSE-2.0\">apache.org/licenses</a>"));

        QString buildInfo = QApplication::translate("Application", "<h3>Build Information</h3>"
                                                                   "<p>"
                                                                   "Branch: %1<br>"
                                                                   "Commit: %2<br>"
                                                                   "Build date: %3<br>"
                                                                   "Toolchain: %4 %5 %6<br>"
                                                                   "Target platform: %7"
                                                                   "</p>")
                                .arg(QStringLiteral(APPLICATION_GIT_BRANCH), //
                                     QStringLiteral(APPLICATION_GIT_LAST_COMMIT_HASH), //
                                     QLocale().toString(QDateTime::fromString(QStringLiteral(APPLICATION_BUILD_TIME), Qt::ISODate).toLocalTime()), //
                                     QStringLiteral(APPLICATION_COMPILER_ARCH), //
                                     QStringLiteral(APPLICATION_COMPILER_ID), //
                                     QStringLiteral(APPLICATION_COMPILER_VERSION), //
                                     QStringLiteral(APPLICATION_TRIPLET));

        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        QQuickIcon icon;
        icon.setSource(QUrl("image://appicon/app"));
        icon.setWidth(64);
        icon.setHeight(64);
        QScopedPointer<QObject> object(component.createWithInitialProperties({{"title", tr("About %1").arg(appName)}, {"textFormat", Qt::RichText}, {"text", appName.toHtmlEscaped()}, {"informativeText", aboutInfo + licenseInfo + buildInfo}, {"width", 480}, {"icon", QVariant::fromValue(icon)}}));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto mb = qobject_cast<QWindow *>(object.get());
        if (!mb) {
            qFatal("MessageBoxDialog in SVSCraft.UIComponents is not QWindow");
        }
        mb->setTransientParent(parent);
        mb->show();
        QEventLoop eventLoop;
        OpenUrlHelper openUrlHelper;
        connect(mb, SIGNAL(done(QVariant)), &eventLoop, SLOT(quit()));
        connect(mb, SIGNAL(linkActivated(QString)), &openUrlHelper, SLOT(openUrl(QString)));
        eventLoop.exec();
    }
    void CoreInterface::execAboutQtDialog(QWindow *parent) {
        QMessageBox::aboutQt(parent->property("invisibleCentralWidget").value<QWidget *>());
    }

    static void raiseWindow(QWindow *window) {
        if (window->visibility() == QWindow::Minimized) {
            window->showNormal();
        }
        window->raise(); // TODO: what does the previous QMView::raiseWindow do to the window?
        window->requestActivate();
    }

    void CoreInterface::showHome() {
        qCInfo(lcCoreInterface) << "Show home";
        auto inst = HomeWindowInterface::instance();
        if (inst) {
            qCInfo(lcCoreInterface) << "Home window already exists, raising it";
            raiseWindow(inst->window());
            return;
        }
        qCInfo(lcCoreInterface) << "Creating home window";
        auto windowInterface = HomeWindowInterfaceRegistry::instance()->create();
        QQmlEngine::setObjectOwnership(windowInterface, QQmlEngine::CppOwnership);
    }

    CoreInterface::CoreInterface(QObject *parent) : CoreInterface(*new CoreInterfacePrivate(), parent) {
    }

    CoreInterface::~CoreInterface() {
    }

    CoreInterface::CoreInterface(CoreInterfacePrivate &d, QObject *parent) : CoreInterfaceBase(d, parent) {
        d.init();
    }

}

#include "CoreInterface.moc"
#include "moc_CoreInterface.cpp"
