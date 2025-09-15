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
#include <QtQuickControls2/QQuickStyle>
#include <QtQuick/QQuickImageProvider>

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>

#include <SVSCraftQuick/Theme.h>

#include <QAKQuick/actioniconimageprovider.h>

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

    CorePlugin::CorePlugin() {
        PluginDatabase::qmlEngine()->addImageProvider("appicon", new AppIconImageProvider);
    }

    CorePlugin::~CorePlugin() = default;

    static constexpr char kOpenSettingsArg[] = "--open-settings";
    static constexpr char kNewProjectArg[] = "--new";

    static QQuickWindow *initializeGui(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        if (options.contains(kOpenSettingsArg)) {
            CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_CommandLineSettings);
            ICore::execSettingsDialog("", nullptr);
        }
        CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_DiffScope);
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

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        initializeSingletons();
        initializeImageProviders();
        initializeActions();
        initializeSettings();
        initializeBehaviorPreference();
        initializeWindows();
        initializeColorScheme();
        initializeJumpList();
        initializeHelpContents();

        QApplication::setQuitOnLastWindowClosed(false);

        return true;
    }

    void CorePlugin::extensionsInitialized() {
    }

    bool CorePlugin::delayedInitialize() {
        PluginDatabase::splash()->showMessage(tr("Initializing GUI..."));
        QApplication::setQuitOnLastWindowClosed(true);
        // TODO plugin arguments
        auto args = QApplication::arguments();
        auto win = initializeGui(args, QDir::currentPath(), args);
        connect(win, &QQuickWindow::sceneGraphInitialized, PluginDatabase::splash(), &QWidget::close);
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
        new ICore(this);
        new ApplicationUpdateChecker(this);
        new BehaviorPreference(this);
    }

    void CorePlugin::initializeImageProviders() {
        auto actionIconImageProvider = new QAK::ActionIconImageProvider;
        actionIconImageProvider->setActionFamily(ICore::actionRegistry());
        PluginDatabase::qmlEngine()->addImageProvider("action", actionIconImageProvider);
    }

    void CorePlugin::initializeActions() {
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

    void CorePlugin::initializeSettings() {
        auto sc = ICore::settingCatalog();
        sc->addPage(new GeneralPage);
        auto appearancePage = new AppearancePage;
        appearancePage->addPage(new ColorSchemePage);
        sc->addPage(appearancePage);
        sc->addPage(new MenuPage);
        sc->addPage(new KeyMapPage);
        sc->addPage(new TimeIndicatorPage);
    }

    void CorePlugin::initializeWindows() {
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
                CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_DisableAnimation);
            }
            auto v = 250 * BehaviorPreference::animationSpeedRatio() * (BehaviorPreference::isAnimationEnabled() ? 1 : 0);
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
        QObject::connect(behaviorPreference, &BehaviorPreference::uiBehaviorChanged, [] {
            if (!(BehaviorPreference::uiBehavior() & BehaviorPreference::UB_Frameless)) {
                CoreAchievementsModel::triggerAchievementCompleted(CoreAchievementsModel::Achievement_DisableCustomTitleBar);
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
        PluginDatabase::instance()->addObject("org.diffscope.achievements", new CoreAchievementsModel(this));
        {
            auto component = new QQmlComponent(PluginDatabase::qmlEngine(), "DiffScope.Core", "ColorSchemeWelcomeWizardPage", this);
            if (component->isError()) {
                qFatal() << component->errorString();
            }
            PluginDatabase::instance()->addObject("org.diffscope.welcomewizard.pages", component);
        }
    }

}
