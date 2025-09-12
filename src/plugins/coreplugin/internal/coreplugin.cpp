#include "coreplugin.h"

#ifdef Q_OS_WIN
#   include <atlbase.h>
#   include <ShlObj.h>
#   include <ShObjIdl.h>
#   include <propkey.h>
#   include <propvarutil.h>
#endif

#include <algorithm>

#include <QTimer>
#include <QtGui/QFontDatabase>
#include <QtCore/QDirIterator>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtGui/QFileOpenEvent>
#include <QtGui/QImageReader>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>
#include <QtQuick/QQuickImageProvider>

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>

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
#include <coreplugin/internal/timeindicatorpage.h>
#include <coreplugin/internal/colorschemecollection.h>
#include <coreplugin/internal/applicationupdatechecker.h>
#include <coreplugin/internal/findactionsaddon.h>
#include <coreplugin/internal/editactionsaddon.h>
#include <coreplugin/internal/timelineaddon.h>
#include <coreplugin/internal/projectstartuptimeraddon.h>

static auto getCoreActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_actions);
}

#ifdef Q_OS_MAC
static auto getCoreMacOSActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_macos_actions);
}
#endif

namespace Core::Internal {

    class AppIconImageProvider : public QQuickImageProvider {
    public:
        AppIconImageProvider() : QQuickImageProvider(Image) {
            for (const auto &subDirName : m_iconDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QDir subDir(m_iconDir.filePath(subDirName));
                QList<int> sizes;
                for (const auto &fileInfo : subDir.entryInfoList(QDir::Files)) {
                    static const auto pattern = QRegularExpression(R"(^(\d+)x\d+\.png$)");
                    const auto match = pattern.match(fileInfo.fileName());
                    if (match.hasMatch()) {
                        sizes.append(match.captured(1).toInt());
                    }
                }
                std::ranges::sort(sizes);
                m_iconMap.insert(subDirName, sizes);
            }
        }

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
            auto sizes = m_iconMap.value(id);
            if (sizes.isEmpty()) {
                return {};
            }
            if (!requestedSize.isValid()) {
                size->setWidth(1024);
                size->setHeight(1024);
            } else {
                *size = requestedSize;
            }
            auto pTargetSize = std::ranges::lower_bound(sizes, qMax(size->width(), size->height()));
            auto targetSize = pTargetSize == sizes.end() ? sizes.last() : *pTargetSize;
            QDir subDir(m_iconDir.filePath(id));
            QImageReader reader(subDir.filePath(QString::number(targetSize) + "x" + QString::number(targetSize) + ".png"));
            return reader.read();
        }

    private:
        static const QDir m_iconDir;
        QHash<QString, QList<int>> m_iconMap;
    };
    const QDir AppIconImageProvider::m_iconDir = QDir(":/diffscope/icons");

    static ICore *icore = nullptr;
    static ApplicationUpdateChecker *updateChecker = nullptr;
    static BehaviorPreference *behaviorPreference = nullptr;

    static void waitSplash(QWidget *w) {
        PluginDatabase::splash()->finish(w);
    }

    CorePlugin::CorePlugin() {
        PluginDatabase::qmlEngine()->addImageProvider("appicon", new AppIconImageProvider);
    }

    CorePlugin::~CorePlugin() = default;

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
        addIcon("core.help", "QuestionCircle16Filled");
        addIcon("core.file.new", "DocumentAdd16Filled");
        addIcon("core.file.open", "FolderOpen16Filled");
        addIcon("core.file.save", "Save16Filled");
        addIcon("core.settings", "Settings16Filled");
        addIcon("core.plugins", "PuzzlePiece16Filled");
        addIcon("core.showHomeWindow", "Home16Filled");
        addIcon("core.documentations", "QuestionCircle16Filled");
        addIcon("core.findActions", "Search16Filled");
        addIcon("core.edit.undo", "ArrowUndo16Filled");
        addIcon("core.edit.redo", "ArrowRedo16Filled");
        addIcon("core.edit.cut", "Cut16Filled");
        addIcon("core.edit.copy", "Copy16Filled");
        addIcon("core.edit.paste", "ClipboardPaste16Filled");
        addIcon("core.edit.delete", "Delete16Filled");
        addIcon("core.panel.properties", "TextBulletListSquareEdit20Filled");
        addIcon("core.panel.plugins", "PuzzlePiece16Filled");
        addIcon("core.panel.arrangement", "GanttChart16Filled");
        addIcon("core.panel.mixer", "OptionsVertical16Filled");
        addIcon("core.panel.pianoRoll", "Midi20Filled");
        addIcon("core.panel.notifications", "Alert16Filled");
        addIcon("core.panel.tips", "ChatSparkle16Filled");
        addIcon("core.timeline.goToStart", "Previous16Filled");
        addIcon("core.timeline.goToEnd", "Next16Filled");
    }

    static void initializeSettings() {
        auto sc = ICore::settingCatalog();
        sc->addPage(new GeneralPage);
        auto appearancePage = new AppearancePage;
        appearancePage->addPage(new ColorSchemePage);
        sc->addPage(appearancePage);
        sc->addPage(new MenuPage);
        sc->addPage(new KeyMapPage);
        sc->addPage(new TimeIndicatorPage);
    }

    static void initializeWindows() {
        IHomeWindowRegistry::instance()->attach<HomeAddOn>();
        IProjectWindowRegistry::instance()->attach<WorkspaceAddOn>();
        IProjectWindowRegistry::instance()->attach<ViewVisibilityAddOn>();
        IProjectWindowRegistry::instance()->attach<NotificationAddOn>();
        IHomeWindowRegistry::instance()->attach<FindActionsAddOn>();
        IProjectWindowRegistry::instance()->attach<FindActionsAddOn>();
        IProjectWindowRegistry::instance()->attach<EditActionsAddOn>();
        IProjectWindowRegistry::instance()->attach<TimelineAddOn>();
        IProjectWindowRegistry::instance()->attach<ProjectStartupTimerAddOn>();
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

#ifdef Q_OS_WIN
    static void initializeJumpList() {
        CoInitialize(nullptr);
        
        CComPtr<ICustomDestinationList> pcdl;
        HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        UINT cMinSlots;
        CComPtr<IObjectArray> poaRemoved;
        hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        CComPtr<IObjectCollection> poc;
        hr = poc.CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        CComPtr<IShellLink> psl;
        hr = psl.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
        if (FAILED(hr)) {
            CoUninitialize();
            return;
        }

        auto appPath = QApplication::applicationFilePath().toStdWString();
        psl->SetPath(appPath.c_str());
        psl->SetArguments(L"--new");
        
        CComPtr<IPropertyStore> pps;
        hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
        if (SUCCEEDED(hr)) {
            PROPVARIANT propvar;
            InitPropVariantFromString(L"New Project", &propvar);
            pps->SetValue(PKEY_Title, propvar);
            PropVariantClear(&propvar);
            pps->Commit();
        }

        poc->AddObject(psl);

        CComPtr<IObjectArray> poa;
        hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
        if (SUCCEEDED(hr)) {
            pcdl->AddUserTasks(poa);
        }

        pcdl->CommitList();
        CoUninitialize();
    }
#endif


    static constexpr char kOpenSettingsArg[] = "--open-settings";
    static constexpr char kNewProjectArg[] = "--new";

    static QQuickWindow *initializeGui(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        if (options.contains(kOpenSettingsArg)) {
            ICore::execSettingsDialog("", nullptr);
        }
        QQuickWindow *win;
        if (options.contains(kNewProjectArg) || (BehaviorPreference::startupBehavior() & BehaviorPreference::SB_CreateNewProject)) {
            win = ICore::newFile();
        } else {
            ICore::showHome();
            win = static_cast<QQuickWindow *>(IHomeWindow::instance()->window());
        }
        if (win->visibility() == QWindow::Minimized) {
            win->showNormal();
        }
        win->raise();
        win->requestActivate();
        return win;
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
#ifdef Q_OS_WIN
        initializeJumpList();
#endif

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
        auto args = ExtensionSystem::PluginManager::arguments();
        auto win = initializeGui(args, QDir::currentPath(), args);
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
        initializeGui(options, workingDirectory, args);
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
