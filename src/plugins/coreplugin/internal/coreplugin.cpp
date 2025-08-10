#include "coreplugin.h"

#include "behaviorpreference.h"

#include <QTimer>
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

#include <CoreApi/plugindatabase.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/windowsystem.h>

#include <loadapi/initroutine.h>

#include <coreplugin/icore.h>
#include <coreplugin/behaviorpreference.h>
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


namespace Core::Internal {

    static ICore *icore = nullptr;

    static void waitSplash(QWidget *w) {
        PluginDatabase::splash()->finish(w);
    }

    CorePlugin::CorePlugin() {
    }

    CorePlugin::~CorePlugin() {
    }

    static auto getCoreActionExtension() {
        return QAK_STATIC_ACTION_EXTENSION(core_actions);
    }

    static void initializeActions() {
        ICore::actionRegistry()->addExtension(getCoreActionExtension());
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
    }

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        PluginDatabase::splash()->showMessage(tr("Initializing core plugin..."));

        QQuickStyle::setStyle("SVSCraft.UIComponents");
        QQuickStyle::setFallbackStyle("Basic");

        // qApp->setWindowIcon(QIcon(":/svg/app/diffsinger.svg"));

        // Init ICore instance
        icore = new ICore(this);

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        initializeActions();
        initializeSettings();
        initializeWindows();

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
            ICore::showSettingsDialog("", nullptr);
        }

        QQuickWindow *win;
        if (ICore::behaviorPreference()->startupBehavior() & BehaviorPreference::SB_CreateNewProject) {
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
