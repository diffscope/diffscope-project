#include "coreplugin.h"

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

#include <loadapi/initroutine.h>

#include <coreplugin/ihomewindow.h>
#include <coreplugin/iprojectwindow.h>
#include <coreplugin/internal/appearancepage.h>
#include <coreplugin/internal/homeaddon.h>
#include <coreplugin/internal/projectaddon.h>

#include "icore.h"


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

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        PluginDatabase::splash()->showMessage(tr("Initializing core plugin..."));

        QQuickStyle::setStyle("SVSCraft.UIComponents");
        QQuickStyle::setFallbackStyle("Basic");

        // qApp->setWindowIcon(QIcon(":/svg/app/diffsinger.svg"));

        // Init ICore instance
        icore = new ICore(this);

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        icore->actionRegistry()->addExtension(getCoreActionExtension());
        // TODO: move to icon manifest later
        const auto addIcon = [&](const QString &id, const QString &iconName) {
            QAK::ActionIcon icon;
            icon.addFile(":/diffscope/coreplugin/icons/" + iconName + ".svg");
            icore->actionRegistry()->addIcon("", id, icon);
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

        auto sc = icore->settingCatalog();
        sc->addPage(new AppearancePage);

        IHomeWindowRegistry::instance()->attach<HomeAddon>();
        IProjectWindowRegistry::instance()->attach<ProjectAddon>();

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

        auto win = static_cast<QQuickWindow *>(IHomeWindowRegistry::instance()->create()->window());
        win->show();
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
