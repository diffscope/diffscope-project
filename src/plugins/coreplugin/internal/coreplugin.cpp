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
#include <QtQml/QQmlComponent>
#include <QtWidgets/QPushButton>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickImageProvider>

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>

#include <SVSCraftQuick/Theme.h>

#include <QAKQuick/actioniconimageprovider.h>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/windowsystem.h>
#include <CoreApi/translationmanager.h>
#include <CoreApi/applicationinfo.h>

#include <loadapi/initroutine.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/internal/behaviorpreference.h>
#include <coreplugin/homewindowinterface.h>
#include <coreplugin/projectwindowinterface.h>
#include <coreplugin/internal/appearancepage.h>
#include <coreplugin/internal/homeaddon.h>
#include <coreplugin/internal/workspaceaddon.h>
#include <coreplugin/internal/viewvisibilityaddon.h>
#include <coreplugin/internal/notificationaddon.h>
#include <coreplugin/internal/generalpage.h>
#include <coreplugin/internal/logpage.h>
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
#include <coreplugin/internal/coreachievementsmodel.h>

static auto getCoreActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_actions);
}

#ifdef Q_OS_MAC
static auto getCoreMacOSActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(core_macos_actions);
}
#endif

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcCorePlugin, "diffscope.core.coreplugin")
    Q_STATIC_LOGGING_CATEGORY(lcAppIconImageProvider, "diffscope.core.appiconimageprovider")

    class AppIconImageProvider : public QQuickImageProvider {
    public:
        AppIconImageProvider() : QQuickImageProvider(Image) {
            for (const auto &subDirName : m_iconDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QDir subDir(m_iconDir.filePath(subDirName));
                qCDebug(lcAppIconImageProvider) << "Processing icon directory:" << subDir.path();
                QList<int> sizes;
                for (const auto &fileInfo : subDir.entryInfoList(QDir::Files)) {
                    qCDebug(lcAppIconImageProvider) << "Processing icon file:" << fileInfo.fileName();
                    static const auto pattern = QRegularExpression(R"(^(\d+)x\d+\.png$)");
                    const auto match = pattern.match(fileInfo.fileName());
                    if (match.hasMatch()) {
                        qCDebug(lcAppIconImageProvider) << "Found icon size:" << match.captured(1);
                        sizes.append(match.captured(1).toInt());
                    }
                }
                std::ranges::sort(sizes);
                m_iconMap.insert(subDirName, sizes);
            }
        }

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
            qCDebug(lcAppIconImageProvider) << "Requesting icon:" << id << requestedSize;
            auto sizes = m_iconMap.value(id);
            if (sizes.isEmpty()) {
                qCWarning(lcAppIconImageProvider) << "No icon found for:" << id;
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

    CorePlugin::CorePlugin() {
        RuntimeInterface::qmlEngine()->addImageProvider("appicon", new AppIconImageProvider);
    }

    CorePlugin::~CorePlugin() = default;

    static constexpr char kOpenSettingsArg[] = "--open-settings";
    static constexpr char kNewProjectArg[] = "--new";

    static void checkUltimateSimplicityAchievement() {
        using namespace ExtensionSystem;
        bool ok = std::ranges::all_of(PluginManager::plugins(), [](PluginSpec *spec) {
            return !spec->isEffectivelyEnabled() || spec->name() == "Core" || spec->name() == "Achievement";
        });
        if (!ok)
            return;
        CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_UltimateSimplicity);
    }

    static QQuickWindow *initializeGui(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        if (options.contains(kOpenSettingsArg)) {
            qCInfo(lcCorePlugin) << "Open settings dialog with command line args";
            CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_CommandLineSettings);
            CoreInterface::execSettingsDialog("", nullptr);
        }
        CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_DiffScope);
        QQuickWindow *win;
        if (options.contains(kNewProjectArg) || (BehaviorPreference::startupBehavior() & BehaviorPreference::SB_CreateNewProject)) {
            qCInfo(lcCorePlugin) << "Create new project on startup";
            win = CoreInterface::newFile();
        } else {
            qCInfo(lcCorePlugin) << "Open home window on startup";
            CoreInterface::showHome();
            win = static_cast<QQuickWindow *>(HomeWindowInterface::instance()->window());
        }
        if (win->visibility() == QWindow::Minimized) {
            win->showNormal();
        }
        win->raise();
        win->requestActivate();
        return win;
    }

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        RuntimeInterface::splash()->showMessage(tr("Initializing core plugin..."));
        qCInfo(lcCorePlugin) << "Initializing";

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        initializeSingletons();
        initializeBehaviorPreference();
        initializeImageProviders();
        initializeActions();
        initializeSettings();
        initializeWindows();
        initializeColorScheme();
        initializeJumpList();
        initializeHelpContents();

        QApplication::setQuitOnLastWindowClosed(false);

        qCInfo(lcCorePlugin) << "Initialized";

        return true;
    }

    void CorePlugin::extensionsInitialized() {
        auto settings = RuntimeInterface::settings();
        settings->setValue("lastInitializationAbortedFlag", false);
    }

    bool CorePlugin::delayedInitialize() {
        RuntimeInterface::splash()->showMessage(tr("Initializing GUI..."));
        qCInfo(lcCorePlugin) << "Initializing GUI";
        QApplication::setQuitOnLastWindowClosed(true);
        // TODO plugin arguments
        auto args = QApplication::arguments();
        auto win = initializeGui(args, QDir::currentPath(), args);
        connect(win, &QQuickWindow::sceneGraphInitialized, RuntimeInterface::splash(), &QWidget::close);
        checkUltimateSimplicityAchievement();
        return false;
    }

    QObject *CorePlugin::remoteCommand(const QStringList &options, const QString &workingDirectory,
                                       const QStringList &args) {
        initializeGui(options, workingDirectory, args);
        return nullptr;
    }

    bool CorePlugin::eventFilter(QObject *obj, QEvent *event) {
        // TODO open file
        return QObject::eventFilter(obj, event);
    }

#ifdef Q_OS_WIN
    static void initializeWindowsJumpList() {
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

#ifdef Q_OS_MACOS
    static void initializeMacOSJumpList() {
    }
#endif

    void CorePlugin::initializeSingletons() {
        new CoreInterface(this);
        new ApplicationUpdateChecker(this);
        new BehaviorPreference(this);
    }

    void CorePlugin::initializeImageProviders() {
        auto actionIconImageProvider = new QAK::ActionIconImageProvider;
        actionIconImageProvider->setActionFamily(CoreInterface::actionRegistry());
        RuntimeInterface::qmlEngine()->addImageProvider("action", actionIconImageProvider);
    }

    void CorePlugin::initializeActions() {
        CoreInterface::actionRegistry()->addExtension(::getCoreActionExtension());
#ifdef Q_OS_MAC
        CoreInterface::actionRegistry()->addExtension(::getCoreMacOSActionExtension());
#endif

        // TODO: move to icon manifest later
        const auto addIcon = [&](const QString &id, const QString &iconName) {
            QAK::ActionIcon icon;
            icon.addFile(":/diffscope/coreplugin/icons/" + iconName + ".svg");
            CoreInterface::actionRegistry()->addIcon("", id, icon);
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

    void CorePlugin::initializeSettings() const {
        GeneralPage::setCorePluginTranslationsPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto sc = CoreInterface::settingCatalog();
        auto generalPage = new GeneralPage;
        generalPage->addPage(new LogPage);
        sc->addPage(generalPage);
        auto appearancePage = new AppearancePage;
        appearancePage->addPage(new ColorSchemePage);
        sc->addPage(appearancePage);
        sc->addPage(new MenuPage);
        sc->addPage(new KeyMapPage);
        sc->addPage(new TimeIndicatorPage);
    }

    void CorePlugin::initializeWindows() {
        HomeWindowInterfaceRegistry::instance()->attach<HomeAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<WorkspaceAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<ViewVisibilityAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<NotificationAddOn>();
        HomeWindowInterfaceRegistry::instance()->attach<FindActionsAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<FindActionsAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<EditActionsAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<TimelineAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<ProjectStartupTimerAddOn>();
    }

    void CorePlugin::initializeBehaviorPreference() {
        auto behaviorPreference = BehaviorPreference::instance();
        const auto updateFont = [=] {
            if (!BehaviorPreference::useCustomFont()) {
                auto font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
                QApplication::setFont(font);
                SVS::Theme::defaultTheme()->setFont(font);
            } else {
                auto font = QApplication::font();
                font.setFamily(BehaviorPreference::fontFamily());
                font.setStyleName(BehaviorPreference::fontStyle());
                QApplication::setFont(font);
                SVS::Theme::defaultTheme()->setFont(font);
            }
        };
        QObject::connect(behaviorPreference, &BehaviorPreference::useCustomFontChanged, updateFont);
        QObject::connect(behaviorPreference, &BehaviorPreference::fontFamilyChanged, updateFont);
        QObject::connect(behaviorPreference, &BehaviorPreference::fontStyleChanged, updateFont);
        const auto updateAnimation = [=] {
            if (!BehaviorPreference::isAnimationEnabled()) {
                CoreAchievementsModel::triggerAchievementCompleted(
                    CoreAchievementsModel::Achievement_DisableAnimation);
            }
            auto v = 250 * BehaviorPreference::animationSpeedRatio() *
                     (BehaviorPreference::isAnimationEnabled() ? 1 : 0);
            SVS::Theme::defaultTheme()->setColorAnimationDuration(static_cast<int>(v));
            SVS::Theme::defaultTheme()->setVisualEffectAnimationDuration(static_cast<int>(v));
        };
        QObject::connect(behaviorPreference, &BehaviorPreference::animationEnabledChanged,
                         updateAnimation);
        QObject::connect(behaviorPreference, &BehaviorPreference::animationSpeedRatioChanged,
                         updateAnimation);
        QObject::connect(behaviorPreference,
                         &BehaviorPreference::commandPaletteClearHistoryRequested, [] {
                             auto settings = RuntimeInterface::settings();
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
        QObject::connect(behaviorPreference, &BehaviorPreference::uiBehaviorChanged, [] {
            if (!(BehaviorPreference::uiBehavior() & BehaviorPreference::UB_Frameless)) {
                CoreAchievementsModel::triggerAchievementCompleted(
                    CoreAchievementsModel::Achievement_DisableCustomTitleBar);
            }
        });
    }

    void CorePlugin::initializeColorScheme() {
        ColorSchemeCollection collection;
        collection.load();
        collection.applyTo(SVS::Theme::defaultTheme(), nullptr); // TODO: ScopicFlow editing area palette
    }

    void CorePlugin::initializeJumpList() {
#ifdef Q_OS_WIN
        initializeWindowsJumpList();
#elif defined(Q_OS_MACOS)
        initializeMacOSJumpList();
#endif
    }

    void CorePlugin::initializeHelpContents() {
        RuntimeInterface::instance()->addObject("org.diffscope.achievements", new CoreAchievementsModel(this));
        {
            auto component = new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ColorSchemeWelcomeWizardPage", this);
            if (component->isError()) {
                qFatal() << component->errorString();
            }
            RuntimeInterface::instance()->addObject("org.diffscope.welcomewizard.pages", component);
        }
    }

}
