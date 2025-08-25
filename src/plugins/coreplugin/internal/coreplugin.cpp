#include "coreplugin.h"

#include "behaviorpreference.h"

#include <QTimer>
#include <QtGui/QFontDatabase>
#include <QtCore/QDirIterator>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtGui/QFileOpenEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>

#include <extensionsystem/pluginspec.h>

#include <SVSCraftQuick/Theme.h>

#include <CoreApi/plugindatabase.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/windowsystem.h>

#include <loadapi/initroutine.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/behaviorpreference.h>
#include <coreplugin/ihomewindow.h>
#include <coreplugin/iprojectwindow.h>
#include <coreplugin/internal/appearancepage.h>
#include <coreplugin/internal/homeaddon.h>
#include <coreplugin/internal/workspaceaddon.h>
#include <coreplugin/internal/viewvisibilityaddon.h>
#include <coreplugin/internal/notificationaddon.h>
#include <coreplugin/internal/generalpage.h>
#include <coreplugin/internal/colorschemepage.h>
#include <coreplugin/internal/keymappage.h>
#include <coreplugin/internal/menupage.h>
#include <coreplugin/internal/colorschemecollection.h>
#include <coreplugin/internal/applicationupdatechecker.h>
#include <coreplugin/internal/findactionsaddon.h>

static auto getCoreActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_actions);
}

#ifdef Q_OS_MAC
static auto getCoreMacOSActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_macos_actions);
}
#endif

namespace Core::Internal {

    static ICore *icore = nullptr;
    static ApplicationUpdateChecker *updateChecker = nullptr;
    static BehaviorPreference *behaviorPreference = nullptr;

    static void waitSplash(QWidget *w) {
        PluginDatabase::splash()->finish(w);
    }

    CorePlugin::CorePlugin() {
    }

    CorePlugin::~CorePlugin() {
    }

    static void initializeActions() {
        ICore::actionRegistry()->addExtension(::getCoreActionExtension());
#ifdef Q_OS_MAC
        ICore::actionRegistry()->addExtension(::getCoreMacOSActionExtension());
#endif
        // TODO: move to icon manifest later
        const auto addIcon = [&](const QString &id, const QString &iconName) {
            QAK::ActionIcon icon;
            icon.addFile(":/diffscope/coreplugin/icons/" + iconName + ".svg");
            ICore::actionRegistry()->addIcon("", id, icon);
        };
        addIcon("core.homePreferences", "Settings16Filled");
        addIcon("core.homeHelp", "QuestionCircle16Filled");
        addIcon("core.newFile", "DocumentAdd16Filled");
        addIcon("core.openFile", "FolderOpen16Filled");
        addIcon("core.saveFile", "Save16Filled");
        addIcon("core.settings", "Settings16Filled");
        addIcon("core.plugins", "PuzzlePiece16Filled");
        addIcon("core.showHome", "Home16Filled");
        addIcon("core.documentations", "QuestionCircle16Filled");
        addIcon("core.findActions", "Search16Filled");
        addIcon("core.propertiesPanel", "TextBulletListSquareEdit20Filled");
        addIcon("core.pluginsPanel", "PuzzlePiece16Filled");
        addIcon("core.arrangementPanel", "GanttChart16Filled");
        addIcon("core.mixerPanel", "OptionsVertical16Filled");
        addIcon("core.pianoRollPanel", "Midi20Filled");
        addIcon("core.notificationsPanel", "Alert16Filled");
        addIcon("core.tipsPanel", "ChatSparkle16Filled");
    }

    static void initializeSettings() {
        auto sc = ICore::settingCatalog();
        sc->addPage(new GeneralPage);
        auto appearancePage = new AppearancePage;
        appearancePage->addPage(new ColorSchemePage);
        sc->addPage(appearancePage);
        sc->addPage(new MenuPage);
        sc->addPage(new KeyMapPage);
    }

    static void initializeWindows() {
        IHomeWindowRegistry::instance()->attach<HomeAddOn>();
        IProjectWindowRegistry::instance()->attach<WorkspaceAddOn>();
        IProjectWindowRegistry::instance()->attach<ViewVisibilityAddOn>();
        IProjectWindowRegistry::instance()->attach<NotificationAddOn>();
        IProjectWindowRegistry::instance()->attach<FindActionsAddOn>();
    }

    static void initializeBehaviorPreference() {
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
        QObject::connect(behaviorPreference, &BehaviorPreference::useCustomFontChanged, updateFont);
        QObject::connect(behaviorPreference, &BehaviorPreference::fontFamilyChanged, updateFont);
        QObject::connect(behaviorPreference, &BehaviorPreference::fontStyleChanged, updateFont);
        const auto updateAnimation = [=] {
            auto v = 250 * behaviorPreference->animationSpeedRatio() * (behaviorPreference->isAnimationEnabled() ? 1 : 0);
            SVS::Theme::defaultTheme()->setColorAnimationDuration(static_cast<int>(v));
            SVS::Theme::defaultTheme()->setVisualEffectAnimationDuration(static_cast<int>(v));
        };
        QObject::connect(behaviorPreference, &BehaviorPreference::animationEnabledChanged, updateAnimation);
        QObject::connect(behaviorPreference, &BehaviorPreference::animationSpeedRatioChanged, updateAnimation);
        QObject::connect(behaviorPreference, &BehaviorPreference::commandPaletteClearHistoryRequested, [] {
            auto settings = PluginDatabase::settings();
            settings->beginGroup(FindActionsAddOn::staticMetaObject.className());
            settings->setValue("priorityActions", QStringList());
            settings->endGroup();
        });
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

    static void initializeColorScheme() {
        ColorSchemeCollection collection;
        collection.load();
        collection.applyTo(SVS::Theme::defaultTheme(), nullptr); // TODO: ScopicFlow editing area palette
    }

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        PluginDatabase::splash()->showMessage(tr("Initializing core plugin..."));

        QQuickStyle::setStyle("SVSCraft.UIComponents");
        QQuickStyle::setFallbackStyle("Basic");

        // qApp->setWindowIcon(QIcon(":/svg/app/diffsinger.svg"));

        // Init ICore instance
        icore = new ICore(this);

        // Init ApplicationUpdateChecker instance
        updateChecker = new ApplicationUpdateChecker(this);

        behaviorPreference = new BehaviorPreference(this);

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        initializeActions();
        initializeSettings();
        initializeBehaviorPreference();
        initializeWindows();
        initializeColorScheme();

        return true;
    }

    void CorePlugin::extensionsInitialized() {
    }

    bool CorePlugin::delayedInitialize() {
        PluginDatabase::splash()->showMessage(tr("Initializing GUI..."));

        // if (auto entry = InitRoutine::startEntry()) {
        //     waitSplash(entry());
        //     return false;
        // }

        if (QApplication::arguments().contains("-open-settings")) {
            ICore::execSettingsDialog("", nullptr);
        }

        QQuickWindow *win;
        if (BehaviorPreference::startupBehavior() & BehaviorPreference::SB_CreateNewProject) {
            ICore::newFile();
            win = static_cast<QQuickWindow *>(ICore::windowSystem()->firstWindowOfType<IProjectWindow>()->window());
        } else {
            ICore::showHome();
            win = static_cast<QQuickWindow *>(IHomeWindow::instance()->window());
        }
        connect(win, &QQuickWindow::sceneGraphInitialized, PluginDatabase::splash(), &QWidget::close);

        return false;
    }

    QObject *CorePlugin::remoteCommand(const QStringList &options, const QString &workingDirectory,
                                       const QStringList &args) {
        // auto firstHandle = icore->windowSystem()->firstWindow();
        // int cnt = openFileFromCommand(workingDirectory, args, firstHandle);
        // if (firstHandle && cnt == 0) {
        //     QMView::raiseWindow(firstHandle->window());
        // }
        return nullptr;
    }

    bool CorePlugin::eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            // openFileFromCommand({}, {static_cast<QFileOpenEvent *>(event)->file()},
            //                     icore->windowSystem()->firstWindow());
        }
        return QObject::eventFilter(obj, event);
    }

}
