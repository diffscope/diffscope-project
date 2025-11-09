#include "CoreInterface.h"

#include <application_buildinfo.h>
#include <application_config.h>

#include <algorithm>
#include <csignal>
#include <memory>

#include <QApplication>
#include <QChildEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QSettings>

#include <QtQuickTemplates2/private/qquickicon_p.h>

#include <QAKCore/actionregistry.h>

#include <CoreApi/filelocker.h>
#include <CoreApi/private/coreinterfacebase_p.h>
#include <CoreApi/recentfilecollection.h>
#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <extensionsystem/pluginmanager.h>

#include <opendspx/model.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/Theme.h>

#include <coreplugin/DspxCheckerRegistry.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/CoreAchievementsModel.h>
#include <coreplugin/internal/ProjectStartupTimerAddOn.h>
#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcCoreInterface, "diffscope.core.coreinterface")

    class CoreInterfacePrivate : CoreInterfaceBasePrivate {
        Q_DECLARE_PUBLIC(CoreInterface)
    public:
        CoreInterfacePrivate() {
        }

        QQmlEngine *qmlEngine;
        QAK::ActionRegistry *actionRegistry;
        DspxCheckerRegistry *dspxCheckerRegistry;

        void init() {
            Q_Q(CoreInterface);
            qmlEngine = new QQmlEngine(q);
            actionRegistry = new QAK::ActionRegistry(q);
            dspxCheckerRegistry = new DspxCheckerRegistry(q);
        }
    };

    class OpenUrlHelper : public QObject {
        Q_OBJECT
    public:
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

    DspxCheckerRegistry * CoreInterface::dspxCheckerRegistry() {
        Q_ASSERT(instance());
        return instance()->d_func()->dspxCheckerRegistry;
    }

    int CoreInterface::execSettingsDialog(const QString &id, QWindow *parent) {
        static std::unique_ptr<QWindow> dlg;

        qCInfo(lcCoreInterface) << "Opening settings dialog" << id;

        // TODO: show last used page if id is empty

        auto settings = RuntimeInterface::settings();
        settings->beginGroup("DiffScope.Core.SettingDialog");
        settings->setValue("currentId", settings->value("currentId", "org.diffscope.core.General"));
        qCDebug(lcCoreInterface) << "Saved current id" << settings->value("currentId").toString();
        settings->endGroup();

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
            dlg.reset(qobject_cast<QWindow *>(component.create()));
            Q_ASSERT(dlg);
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
        Internal::CoreAchievementsModel::triggerAchievementCompleted(Internal::CoreAchievementsModel::Achievement_Plugins);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PluginDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> dlg(qobject_cast<QWindow *>(component.create()));
        Q_ASSERT(dlg);
        dlg->setTransientParent(parent);
        dlg->show();
        QEventLoop eventLoop;
        connect(dlg.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
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
            )
                .arg(QStringLiteral("<a "
                                    "href=\"https://www.apache.org/licenses/"
                                    "LICENSE-2.0\">apache.org/licenses</a>"));

        QString buildInfo = QApplication::translate("Application", "<h3>Build Information</h3>"
                                                                   "<p>"
                                                                   "Branch: %1<br>"
                                                                   "Commit: %2<br>"
                                                                   "Build date: %3<br>"
                                                                   "Toolchain: %4 %5 %6"
                                                                   "</p>")
                                .arg(QStringLiteral(APPLICATION_GIT_BRANCH), //
                                     QStringLiteral(APPLICATION_GIT_LAST_COMMIT_HASH), //
                                     QLocale().toString(QDateTime::fromString(QStringLiteral(APPLICATION_BUILD_TIME), Qt::ISODate).toLocalTime()), //
                                     QStringLiteral(APPLICATION_COMPILER_ARCH), //
                                     QStringLiteral(APPLICATION_COMPILER_ID), //
                                     QStringLiteral(APPLICATION_COMPILER_VERSION));

        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        QQuickIcon icon;
        icon.setSource(QUrl("image://appicon/app"));
        icon.setWidth(64);
        icon.setHeight(64);
        QScopedPointer mb(qobject_cast<QWindow *>(component.createWithInitialProperties({{"title", tr("About %1").arg(appName)}, {"textFormat", Qt::RichText}, {"text", appName.toHtmlEscaped()}, {"informativeText", aboutInfo + licenseInfo + buildInfo}, {"width", 480}, {"icon", QVariant::fromValue(icon)}})));
        Q_ASSERT(mb);
        mb->setTransientParent(parent);
        mb->show();
        QEventLoop eventLoop;
        OpenUrlHelper openUrlHelper;
        connect(mb.get(), SIGNAL(done(QVariant)), &eventLoop, SLOT(quit()));
        connect(mb.get(), SIGNAL(linkActivated(QString)), &openUrlHelper, SLOT(openUrl(QString)));
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

    static ProjectWindowInterface *createProjectWindow(ProjectDocumentContext *projectDocumentContext) {
        Internal::ProjectStartupTimerAddOn::startTimer();
        auto windowInterface = ProjectWindowInterfaceRegistry::instance()->create(projectDocumentContext);
        QQmlEngine::setObjectOwnership(windowInterface, QQmlEngine::CppOwnership);
        projectDocumentContext->setParent(windowInterface);
        auto win = static_cast<QQuickWindow *>(windowInterface->window());
        win->show();
        if (HomeWindowInterface::instance() && (Internal::BehaviorPreference::startupBehavior() & Internal::BehaviorPreference::SB_CloseHomeWindowAfterOpeningProject)) {
            qCInfo(lcCoreInterface) << "Closing home window";
            HomeWindowInterface::instance()->quit();
        }
        return windowInterface;
    }

    static void triggerAchievementAfterNewProjectWindowOpened(QWindow *win) {
        class ExposedListener : public QObject {
        public:
            explicit ExposedListener(QObject *parent) : QObject(parent) {
            }

            bool eventFilter(QObject *watched, QEvent *event) override {
                if (event->type() == QEvent::Expose && static_cast<QWindow *>(watched)->isExposed()) {
                    Internal::CoreAchievementsModel::triggerAchievementCompleted(Internal::CoreAchievementsModel::Achievement_NewProject);
                    deleteLater();
                }
                return QObject::eventFilter(watched, event);
            }
        };
        auto listener = new ExposedListener(win);
        win->installEventFilter(listener);
    }

    ProjectWindowInterface *CoreInterface::newFile(QWindow *parent) {
        static QDspx::Model defaultModel{
            .version = QDspx::Model::V1,
            .content = {

            }
        };
        qCInfo(lcCoreInterface) << "New file";
        auto projectDocumentContext = std::make_unique<ProjectDocumentContext>();
        projectDocumentContext->newFile(defaultModel, false);
        auto windowInterface = createProjectWindow(projectDocumentContext.release());
        triggerAchievementAfterNewProjectWindowOpened(windowInterface->window());
        return windowInterface;
    }

    ProjectWindowInterface *CoreInterface::newFileFromTemplate(const QString &templateFilePath_, QWindow *parent) {
        qCInfo(lcCoreInterface) << "New file from template" << templateFilePath_;
        auto projectDocumentContext = std::make_unique<ProjectDocumentContext>();
        projectDocumentContext->openSaveProjectFileScenario()->setWindow(parent);
        auto templateFilePath = templateFilePath_;
        if (templateFilePath.isEmpty()) {
            templateFilePath = projectDocumentContext->openSaveProjectFileScenario()->openProjectFile();
            if (templateFilePath.isEmpty())
                return nullptr;
        }
        if (!projectDocumentContext->newFile(templateFilePath, false)) {
            return nullptr;
        }
        auto windowInterface = createProjectWindow(projectDocumentContext.release());
        triggerAchievementAfterNewProjectWindowOpened(windowInterface->window());
        return windowInterface;
    }

    ProjectWindowInterface *CoreInterface::openFile(const QString &filePath_, QWindow *parent) {
        qCInfo(lcCoreInterface) << "Open file" << filePath_;
        auto projectDocumentContext = std::make_unique<ProjectDocumentContext>();
        projectDocumentContext->openSaveProjectFileScenario()->setWindow(parent);
        auto filePath = filePath_;
        if (filePath.isEmpty()) {
            filePath = projectDocumentContext->openSaveProjectFileScenario()->openProjectFile();
            if (filePath.isEmpty())
                return nullptr;
        }

        auto windows = windowSystem()->windows();
        auto openedWindow = std::ranges::find_if(windows, [filePath](WindowInterface *windowInterface) {
            if (auto projectWindowInterface = qobject_cast<ProjectWindowInterface *>(windowInterface)) {
                if (!projectWindowInterface->projectDocumentContext()->fileLocker() || projectWindowInterface->projectDocumentContext()->fileLocker()->path().isEmpty())
                    return false;
                return QFileInfo(projectWindowInterface->projectDocumentContext()->fileLocker()->path()).canonicalFilePath() == QFileInfo(filePath).canonicalFilePath();
            }
            return false;
        });
        if (openedWindow != windows.end()) {
            qCInfo(lcCoreInterface) << "File already opened" << filePath;
            raiseWindow((*openedWindow)->window());
            return qobject_cast<ProjectWindowInterface *>(*openedWindow);
        }
        if (!projectDocumentContext->openFile(filePath)) {
            return nullptr;
        }
        auto windowInterface = createProjectWindow(projectDocumentContext.release());
        recentFileCollection()->addRecentFile(filePath, {});
        return windowInterface;
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
