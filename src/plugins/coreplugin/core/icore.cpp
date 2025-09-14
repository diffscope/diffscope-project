#include "icore.h"
#include "icore.h"

#include <csignal>
#include <memory>

#include "QStyleFactory"
#include <QApplication>
#include <QChildEvent>
#include <QMessageBox>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QGridLayout>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDesktopServices>
#include <QQuickWindow>
#include <QFontDatabase>

#include <QtQuickTemplates2/private/qquickicon_p.h>

#include <extensionsystem/pluginmanager.h>

#include <application_buildinfo.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/Theme.h>

#include <CoreApi/private/icorebase_p.h>
#include <CoreApi/plugindatabase.h>

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/ihomewindow.h>
#include <coreplugin/internal/behaviorpreference.h>
#include <coreplugin/internal/applicationupdatechecker.h>
#include <coreplugin/internal/projectstartuptimeraddon.h>

namespace Core {

    class ICorePrivate : ICoreBasePrivate {
        Q_DECLARE_PUBLIC(ICore)
    public:
        ICorePrivate() {
        }

        QQmlEngine *qmlEngine;
        QAK::ActionRegistry *actionRegistry;

        void init() {
            Q_Q(ICore);
            qmlEngine = new QQmlEngine(q);
            actionRegistry = new QAK::ActionRegistry(q);
        }
    };

    class OpenUrlHelper: public QObject {
        Q_OBJECT
    public:
        Q_INVOKABLE static inline void openUrl(const QString &url) {
            QDesktopServices::openUrl(QUrl(url));
        }
    };

    ICore *ICore::instance() {
        return static_cast<ICore *>(ICoreBase::instance());
    }
    QAK::ActionRegistry *ICore::actionRegistry() {
        if (!instance())
            return nullptr;
        return instance()->d_func()->actionRegistry;
    }

    int ICore::execSettingsDialog(const QString &id, QWindow *parent) {
        static std::unique_ptr<QWindow> dlg;

        // TODO: show last used page if id is empty

        auto settings = PluginDatabase::settings();
        settings->beginGroup("DiffScope.Core.SettingDialog");
        settings->setValue("currentId", settings->value("currentId", "core.General"));
        settings->endGroup();

        if (dlg) {
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            return -1;
        }

        int code;
        {
            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "SettingDialog");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            dlg.reset(qobject_cast<QWindow *>(component.create()));
            Q_ASSERT(dlg);
            dlg->setTransientParent(parent);
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            dlg->show();
            QEventLoop eventLoop;
            connect(dlg.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            dlg.reset();
        }

        // return code;
        return 0;
    }

    void ICore::execPluginsDialog(QWindow *parent) {
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.Core", "PluginDialog");
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
    void ICore::execAboutAppDialog(QWindow *parent) {
        static const QString appName = qApp->applicationName();

        QString aboutInfo =
            QApplication::translate(
                "Application",
                "<p>A professional singing-voice-synthesis editor powered by DiffSinger</p>"
                "<p>Visit <a href=\"https://diffscope.org/\">diffscope.org</a> for more information.</p>");

        QString copyrightInfo =
            QApplication::translate(
                "Application", "<p>Based on Qt version %1.<br>"
                               "Copyright \u00a9 2019-%2 Team OpenVPI. All rights reserved.</p>")
                .arg(QStringLiteral(QT_VERSION_STR), QStringLiteral(APPLICATION_BUILD_YEAR));

        QString licenseInfo =
            QApplication::translate(
                "Application",
                "<h3>License</h3>"
                "<p>Licensed under the Apache License, Version 2.0.<br>"
                "You may obtain a copy of the License at %1.</p>"
                "<p>This application is distributed "
                "<b>AS IS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND</b>, either express or "
                "implied.</p>")
                .arg(QStringLiteral("<a "
                                    "href=\"https://www.apache.org/licenses/"
                                    "LICENSE-2.0\">apache.org/licenses</a>"));

        QString buildInfo = QApplication::translate("Application", "<h3>Build Information</h3>"
                                                                   "<p>"
                                                                   "Version: %1<br>"
                                                                   "Branch: %2<br>"
                                                                   "Commit: %3<br>"
                                                                   "Build date: %4<br>"
                                                                   "Toolchain: %5 %6 %7"
                                                                   "</p>")
                                .arg(QApplication::applicationVersion(),
                                     QStringLiteral(APPLICATION_GIT_BRANCH),           //
                                     QStringLiteral(APPLICATION_GIT_LAST_COMMIT_HASH), //
                                     QStringLiteral(APPLICATION_BUILD_TIME),           //
                                     QStringLiteral(APPLICATION_COMPILER_ARCH),        //
                                     QStringLiteral(APPLICATION_COMPILER_ID),          //
                                     QStringLiteral(APPLICATION_COMPILER_VERSION));

        QQmlComponent component(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        QQuickIcon icon;
        icon.setSource(QUrl("image://appicon/app"));
        icon.setWidth(64);
        icon.setHeight(64);
        QScopedPointer mb(qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"title", tr("About %1").arg(appName)},
            {"textFormat",      Qt::RichText                                       },
            {"text",            appName.toHtmlEscaped()                            },
            {"informativeText", aboutInfo + copyrightInfo + licenseInfo + buildInfo},
            {"width", 480},
            {"icon", QVariant::fromValue(icon)}
        })));
        Q_ASSERT(mb);
        mb->setTransientParent(parent);
        mb->show();
        QEventLoop eventLoop;
        OpenUrlHelper openUrlHelper;
        connect(mb.get(), SIGNAL(done(QVariant)), &eventLoop, SLOT(quit()));
        connect(mb.get(), SIGNAL(linkActivated(QString)), &openUrlHelper, SLOT(openUrl(QString)));
        eventLoop.exec();
    }
    void ICore::execAboutQtDialog(QWindow *parent) {
        QMessageBox::aboutQt(parent->property("invisibleCentralWidget").value<QWidget *>());
    }

    void ICore::showHome() {
        auto inst = IHomeWindow::instance();
        if (inst) {
            if (inst->window()->visibility() == QWindow::Minimized) {
                inst->window()->showNormal();
            }
            inst->window()
                ->raise(); // TODO: what does the previous QMView::raiseWindow do to the window?
            inst->window()->requestActivate();
            return;
        }
        auto iWin = IHomeWindowRegistry::instance()->create();
        Q_UNUSED(iWin);
    }

    void ICore::checkForUpdate(bool silent) {
        Internal::ApplicationUpdateChecker::checkForUpdate(silent);
    }

    QQuickWindow *ICore::newFile() {
        Internal::ProjectStartupTimerAddOn::startTimer();
        // TODO: temporarily creates a project window for testing
        auto win = static_cast<QQuickWindow *>(IProjectWindowRegistry::instance()->create()->window());
        win->show();
        if (IHomeWindow::instance() && (Internal::BehaviorPreference::startupBehavior() & Internal::BehaviorPreference::SB_CloseHomeWindowAfterOpeningProject)) {
            IHomeWindow::instance()->quit();
        }
        return win;
    }

    bool ICore::openFile(const QString &fileName, QWidget *parent) {
        // auto docMgr = ICore::instance()->documentSystem();
        // if (fileName.isEmpty()) {
        //     return docMgr->openFileBrowse(parent, DspxSpec::instance());
        // }
        // return DspxSpec::instance()->open(fileName, parent);
        return false;
    }

    ICore::ICore(QObject *parent) : ICore(*new ICorePrivate(), parent) {
    }

    ICore::~ICore() {
    }

    ICore::ICore(ICorePrivate &d, QObject *parent) : ICoreBase(d, parent) {
        d.init();
    }


}

#include "moc_icore.cpp"
#include "icore.moc"