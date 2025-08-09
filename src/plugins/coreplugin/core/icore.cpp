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

#include <coreplugin/iprojectwindow.h>
#include <coreplugin/ihomewindow.h>
#include <coreplugin/behaviorpreference.h>

namespace Core {

    class ICorePrivate : ICoreBasePrivate {
        Q_DECLARE_PUBLIC(ICore)
    public:
        ICorePrivate() {
        }

        QQmlEngine *qmlEngine;
        QAK::ActionRegistry *actionRegistry;
        BehaviorPreference *behaviorPreference;

        void init() {
            Q_Q(ICore);
            qmlEngine = new QQmlEngine(q);
            actionRegistry = new QAK::ActionRegistry(q);
            behaviorPreference = new BehaviorPreference(q);
            initializeBehaviorPreference();
        }

        void initializeBehaviorPreference() {
            Q_Q(ICore);
            const auto updateFont = [=] {
                if (!behaviorPreference->useCustomFont()) {
                    auto font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
                    QApplication::setFont(font);
                    SVS::Theme::defaultTheme()->setFont(font);
                } else {
                    auto font = QApplication::font();
                    font.setFamily(behaviorPreference->fontFamily());
                    font.setStyleName(behaviorPreference->fontStyle());
                    QApplication::setFont(font);
                    SVS::Theme::defaultTheme()->setFont(font);
                }
            };
            QObject::connect(behaviorPreference, &BehaviorPreference::useCustomFontChanged, q, updateFont);
            QObject::connect(behaviorPreference, &BehaviorPreference::fontFamilyChanged, q, updateFont);
            QObject::connect(behaviorPreference, &BehaviorPreference::fontStyleChanged, q, updateFont);
#ifndef Q_OS_WIN
            QObject::connect(behaviorPreference, &BehaviorPreference::uiBehaviorChanged, q, [=] {
                QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, !(behaviorPreference->uiBehavior() & BehaviorPreference::UB_NativeMenu));
            });
#endif
            const auto updateAnimation = [=] {
                auto v = 250 * behaviorPreference->animationSpeedRatio() * (behaviorPreference->isAnimationEnabled() ? 1 : 0);
                SVS::Theme::defaultTheme()->setColorAnimationDuration(static_cast<int>(v));
                SVS::Theme::defaultTheme()->setVisualEffectAnimationDuration(static_cast<int>(v));
            };
            QObject::connect(behaviorPreference, &BehaviorPreference::animationEnabledChanged, q, updateAnimation);
            QObject::connect(behaviorPreference, &BehaviorPreference::animationSpeedRatioChanged, q, updateAnimation);
            behaviorPreference->load();
            if (!(behaviorPreference->graphicsBehavior() & BehaviorPreference::GB_Hardware)) {
                QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
            }
            if (behaviorPreference->graphicsBehavior() & BehaviorPreference::GB_Antialiasing) {
                auto sf = QSurfaceFormat::defaultFormat();
                sf.setSamples(8);
                QSurfaceFormat::setDefaultFormat(sf);
            }
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
    QQmlEngine *ICore::qmlEngine() {
        if (!instance())
            return nullptr;
        return instance()->d_func()->qmlEngine;
    }
    QAK::ActionRegistry *ICore::actionRegistry() {
        if (!instance())
            return nullptr;
        return instance()->d_func()->actionRegistry;
    }
    BehaviorPreference *ICore::behaviorPreference() {
        if (!instance())
            return nullptr;
        return instance()->d_func()->behaviorPreference;
    }

    int ICore::showSettingsDialog(const QString &id, QWindow *parent) {
        static std::unique_ptr<QWindow> dlg;

        // TODO: show last used page if id is empty

        if (dlg) {
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            return -1;
        }

        int code;
        {
            QQmlComponent component(qmlEngine(), "DiffScope.CorePlugin", "SettingDialog");
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

    void ICore::showPluginsDialog(QWindow *parent) {
        QQmlComponent component(qmlEngine(), "DiffScope.CorePlugin", "PluginDialog");
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
    void ICore::showAboutAppDialog(QWindow *parent) {
        static const QString appName = qApp->applicationName();

        QString aboutInfo =
            QApplication::translate(
                "Application",
                "<p>%1 is a cross-platform SVS editing application powered by "
                "DiffSinger for virtual singer producers to make song compositions.</p>")
                .arg(appName);

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

        QQmlComponent component(qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        QScopedPointer mb(qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"textFormat",      Qt::RichText                                       },
            {"text",            appName.toHtmlEscaped()                            },
            {"informativeText", aboutInfo + copyrightInfo + licenseInfo + buildInfo},
            {"width", 480}
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
    void ICore::showAboutQtDialog(QWindow *parent) {
        QString translatedTextAboutQtCaption;
        translatedTextAboutQtCaption = QMessageBox::tr(
            "<h3>About Qt</h3>"
            "<p>This program uses Qt version %1.</p>"
            ).arg(QT_VERSION_STR);
        //: Leave this text untranslated or include a verbatim copy of it below
        //: and note that it is the authoritative version in case of doubt.
        const QString translatedTextAboutQtText = QMessageBox::tr(
            "<p>Qt is a C++ toolkit for cross-platform application "
            "development.</p>"
            "<p>Qt provides single-source portability across all major desktop "
            "operating systems. It is also available for embedded Linux and other "
            "embedded and mobile operating systems.</p>"
            "<p>Qt is available under multiple licensing options designed "
            "to accommodate the needs of our various users.</p>"
            "<p>Qt licensed under our commercial license agreement is appropriate "
            "for development of proprietary/commercial software where you do not "
            "want to share any source code with third parties or otherwise cannot "
            "comply with the terms of GNU (L)GPL.</p>"
            "<p>Qt licensed under GNU (L)GPL is appropriate for the "
            "development of Qt&nbsp;applications provided you can comply with the terms "
            "and conditions of the respective licenses.</p>"
            "<p>Please see <a href=\"https://%2/\">%2</a> "
            "for an overview of Qt licensing.</p>"
            "<p>Copyright (C) The Qt Company Ltd. and other "
            "contributors.</p>"
            "<p>Qt and the Qt logo are trademarks of The Qt Company Ltd.</p>"
            "<p>Qt is The Qt Company Ltd. product developed as an open source "
            "project. See <a href=\"https://%3/\">%3</a> for more information.</p>"
            ).arg(QStringLiteral("qt.io/licensing"),
                  QStringLiteral("qt.io"));
        QQmlComponent component(qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        QQuickIcon icon;
        icon.setSource(QUrl("qrc:/qt-project.org/qmessagebox/images/qtlogo-64.png"));
        QScopedPointer mb(qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"textFormat",      Qt::RichText},
            {"text",            translatedTextAboutQtCaption},
            {"informativeText", translatedTextAboutQtText},
            {"icon", QVariant::fromValue(icon)},
            {"width", 480}
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

    void ICore::showHome() {
        auto inst = IHomeWindow::instance();
        if (inst) {
            if (inst->window()->visibility() == QWindow::Minimized) {
                inst->window()->showNormal();
            }
            inst->window()->raise(); // TODO: what does the previous QMView::raiseWindow do to the window?
            return;
        }
        auto iWin = IHomeWindowRegistry::instance()->create();
        Q_UNUSED(iWin);
    }
    void ICore::newFile() {
        // TODO: temporarily creates a project window for testing
        auto win = static_cast<QQuickWindow *>(IProjectWindowRegistry::instance()->create()->window());
        win->show();
        if (IHomeWindow::instance() && (behaviorPreference()->startupBehavior() & BehaviorPreference::SB_CloseHomeWindowAfterOpeningProject)) {
            IHomeWindow::instance()->quit();
        }
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