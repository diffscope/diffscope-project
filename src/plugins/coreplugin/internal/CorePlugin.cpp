#include "CorePlugin.h"

#include <algorithm>

#include <QApplication>
#include <QDirIterator>
#include <QFileOpenEvent>
#include <QFontDatabase>
#include <QImageReader>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QPushButton>
#include <QQmlComponent>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QSplashScreen>
#include <QThread>
#include <QTimer>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>
#include <CoreApi/windowsystem.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <SVSCraftQuick/MessageBox.h>
#include <SVSCraftQuick/Theme.h>

#include <loadapi/initroutine.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxClipboard.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/AfterSavingNotifyAddOn.h>
#include <coreplugin/internal/AppearancePage.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/CloseSaveCheckAddOn.h>
#include <coreplugin/internal/ColorSchemeCollection.h>
#include <coreplugin/internal/ColorSchemePage.h>
#include <coreplugin/internal/EditActionsAddOn.h>
#include <coreplugin/internal/FileBackupPage.h>
#include <coreplugin/internal/FindActionsAddOn.h>
#include <coreplugin/internal/GeneralPage.h>
#include <coreplugin/internal/HomeAddOn.h>
#include <coreplugin/internal/InsertItemAddOn.h>
#include <coreplugin/internal/KeymapPage.h>
#include <coreplugin/internal/LogPage.h>
#include <coreplugin/internal/MenuPage.h>
#include <coreplugin/internal/MetadataAddOn.h>
#include <coreplugin/internal/NotificationAddOn.h>
#include <coreplugin/internal/PlatformJumpListHelper.h>
#include <coreplugin/internal/ProjectStartupTimerAddOn.h>
#include <coreplugin/internal/ProjectWindowNavigatorAddOn.h>
#include <coreplugin/internal/PropertiesAddOn.h>
#include <coreplugin/internal/RecentFileAddOn.h>
#include <coreplugin/internal/TimeIndicatorPage.h>
#include <coreplugin/internal/TimelineAddOn.h>
#include <coreplugin/internal/UndoAddOn.h>
#include <coreplugin/internal/ViewVisibilityAddOn.h>
#include <coreplugin/internal/WorkspaceAddOn.h>
#include <coreplugin/PropertyEditorManager.h>

static auto getCoreActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(coreplugin);
}

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
    static constexpr char kNoWarningLastRun[] = "--no-warning-last-run";
    static constexpr char kFatalWithoutPlugins[] = "--fatal-without-plugins";
    static constexpr char kNoWarningPluginError[] = "--no-warning-plugin-error";

    static ActionWindowInterfaceBase *initializeGui(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        qCDebug(lcCorePlugin) << "Initializing GUI" << options << args;
        if (options.contains(kOpenSettingsArg)) {
            qCInfo(lcCorePlugin) << "Open settings dialog with command line args";
            CoreInterface::execSettingsDialog("", nullptr);
        }
        QList<ActionWindowInterfaceBase *> windowInterfaceList;
        do {
            if (!args.isEmpty()) {
                for (const auto &arg : args) {
                    auto filePath = QDir(workingDirectory).absoluteFilePath(arg);
                    auto windowInterface = CoreInterface::openFile(filePath);
                    if (windowInterface) {
                        windowInterfaceList.append(windowInterface);
                    }
                }
                if (!windowInterfaceList.isEmpty()) {
                    break;
                }
            }
            if (options.contains(kNewProjectArg) || (BehaviorPreference::startupBehavior() & BehaviorPreference::SB_CreateNewProject)) {
                qCInfo(lcCorePlugin) << "Create new project on startup";
                windowInterfaceList.append(CoreInterface::newFile());
            } else {
                qCInfo(lcCorePlugin) << "Open home window on startup";
                CoreInterface::showHome();
                windowInterfaceList.append(HomeWindowInterface::instance());
            }
        } while (false);
        for (auto windowInterface : windowInterfaceList) {
            auto win = windowInterface->window();
            if (win->visibility() == QWindow::Minimized) {
                win->showNormal();
            }
            win->raise();
            win->requestActivate();
        }
        return windowInterfaceList.first();
    }

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        RuntimeInterface::splash()->showMessage(tr("Initializing core plugin..."));
        qCInfo(lcCorePlugin) << "Initializing";

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        initializeSingletons();
        initializeBehaviorPreference();
        initializeActions();
        initializeSettings();
        initializeWindows();
        initializeColorScheme();
        initializeJumpList();
        initializeHelpContents();
        initializePropertyEditors();

        QApplication::setQuitOnLastWindowClosed(false);

        qCInfo(lcCorePlugin) << "Initialized";

        return true;
    }

    void CorePlugin::extensionsInitialized() {
        RuntimeInterface::splash()->showMessage(tr("Plugins loading complete, preparing for subsequent initialization..."));
        for (auto plugin : ExtensionSystem::PluginManager::plugins()) {
            qCInfo(lcCorePlugin) << "Plugin" << plugin->name() << "enabled =" << plugin->isEffectivelyEnabled() << "enabledBySettings =" << plugin->isEnabledBySettings() << "hasError =" << plugin->hasError();
        }
        checkLastRun();
        checkPlugins();
    }

    bool CorePlugin::delayedInitialize() {
        RuntimeInterface::splash()->showMessage(tr("Initializing GUI..."));
        qCInfo(lcCorePlugin) << "Initializing GUI";
        // TODO plugin arguments
        auto args = QApplication::arguments();
        auto windowInterface = initializeGui(args, QDir::currentPath(), ExtensionSystem::PluginManager::arguments());
        auto win = windowInterface->window();
        class ExposedListener : public QObject {
        public:
            explicit ExposedListener(QObject *parent) : QObject(parent) {
            }

            bool eventFilter(QObject *watched, QEvent *event) override {
                if (event->type() == QEvent::Expose && static_cast<QWindow *>(watched)->isExposed()) {
                    RuntimeInterface::splash()->close();
                    deleteLater();
                }
                return QObject::eventFilter(watched, event);
            }
        };
        auto listener = new ExposedListener(win);
        win->installEventFilter(listener);
        QApplication::setQuitOnLastWindowClosed(true);
        return false;
    }

    QObject *CorePlugin::remoteCommand(const QStringList &options, const QString &workingDirectory, const QStringList &args) {
        initializeGui(options, workingDirectory, args);
        return nullptr;
    }

    bool CorePlugin::eventFilter(QObject *obj, QEvent *event) {
        // TODO open file
        return QObject::eventFilter(obj, event);
    }

    void CorePlugin::initializeSingletons() {
        new CoreInterface(this);
        new BehaviorPreference(this);
        new DspxClipboard(this);
    }

    void CorePlugin::initializeActions() {
        CoreInterface::actionRegistry()->addExtension(::getCoreActionExtension());
        CoreInterface::actionRegistry()->addIconManifest(":/diffscope/coreplugin/icons/config.json");
    }

    void CorePlugin::initializeSettings() {
        GeneralPage::setCorePluginTranslationsPath(pluginSpec()->location() + QStringLiteral("/translations"));
        auto sc = CoreInterface::settingCatalog();
        auto generalPage = new GeneralPage;
        generalPage->addPage(new LogPage);
        generalPage->addPage(new FileBackupPage);
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
        ProjectWindowInterfaceRegistry::instance()->attach<InsertItemAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<UndoAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<TimelineAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<ProjectStartupTimerAddOn>();
        HomeWindowInterfaceRegistry::instance()->attach<RecentFileAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<RecentFileAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<MetadataAddOn>();
        HomeWindowInterfaceRegistry::instance()->attach<ProjectWindowNavigatorAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<ProjectWindowNavigatorAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<AfterSavingNotifyAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<CloseSaveCheckAddOn>();
        ProjectWindowInterfaceRegistry::instance()->attach<PropertiesAddOn>();

        auto windowSystem = CoreInterface::windowSystem();
        connect(windowSystem, &WindowSystem::windowAboutToDestroy, this, [](WindowInterface *windowInterface) {
            if (!qobject_cast<ProjectWindowInterface *>(windowInterface)) {
                return;
            }

            const auto startupBehavior = BehaviorPreference::startupBehavior();
            const bool closeHomeAfterOpening = startupBehavior & BehaviorPreference::SB_CloseHomeWindowAfterOpeningProject;
            const bool reopenHomeOnLastProjectClosed = startupBehavior & BehaviorPreference::SB_OpenHomeWindowWhenLastProjectClosed;

            if (closeHomeAfterOpening && !reopenHomeOnLastProjectClosed) {
                return;
            }

            const bool hasOtherProjectWindow = std::ranges::any_of(CoreInterface::windowSystem()->windows(), [](auto w) {
                return qobject_cast<ProjectWindowInterface *>(w);
            });

            if (!hasOtherProjectWindow) {
                CoreInterface::showHome();
            }
        });
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
            auto v = 250 * BehaviorPreference::animationSpeedRatio() *
                     (BehaviorPreference::isAnimationEnabled() ? 1 : 0);
            SVS::Theme::defaultTheme()->setColorAnimationDuration(static_cast<int>(v));
            SVS::Theme::defaultTheme()->setVisualEffectAnimationDuration(static_cast<int>(v));
        };
        QObject::connect(behaviorPreference, &BehaviorPreference::animationEnabledChanged, updateAnimation);
        QObject::connect(behaviorPreference, &BehaviorPreference::animationSpeedRatioChanged, updateAnimation);
        QObject::connect(behaviorPreference, &BehaviorPreference::commandPaletteClearHistoryRequested, [] {
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
    }

    void CorePlugin::initializeColorScheme() {
        ColorSchemeCollection collection;
        collection.load();
        collection.applyTo(SVS::Theme::defaultTheme(), nullptr); // TODO: ScopicFlow editing area palette
    }

    void CorePlugin::initializeJumpList() {
        PlatformJumpListHelper::initializePlatformJumpList();
    }

    void CorePlugin::initializeHelpContents() {
        {
            auto component = new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ColorSchemeWelcomeWizardPage", this);
            if (component->isError()) {
                qFatal() << component->errorString();
            }
            RuntimeInterface::instance()->addObject("org.diffscope.welcomewizard.pages", component);
        }
    }

    void CorePlugin::initializePropertyEditors() {
        const auto f = [=, this](const QString &typeName, void(PropertyEditorManager::*addMethod)(QQmlComponent *)) {
            auto component = new QQmlComponent(RuntimeInterface::qmlEngine(), "DiffScope.Core", typeName, this);
            if (component->isError()) {
                qFatal() << component->errorString();
            }
            (CoreInterface::propertyEditorManager()->*addMethod)(component);
        };
        f("MetadataPropertyEditor", &PropertyEditorManager::addNoneComponent);
        f("GlobalCentShiftPropertyEditor", &PropertyEditorManager::addNoneComponent);
        f("MasterControlPropertyEditor", &PropertyEditorManager::addNoneComponent);
        f("LoopPropertyEditor", &PropertyEditorManager::addNoneComponent);

        f("ClipPropertyEditor", &PropertyEditorManager::addClipComponent);
        f("AudioPropertyEditor", &PropertyEditorManager::addClipComponent);
        f("VirtualSingerPropertyEditor", &PropertyEditorManager::addClipComponent);
        f("ControlPropertyEditor", &PropertyEditorManager::addClipComponent);
        f("ClipTimePropertyEditor", &PropertyEditorManager::addClipComponent);

        f("LabelPropertyEditor", &PropertyEditorManager::addLabelComponent);

        f("TempoPropertyEditor", &PropertyEditorManager::addTempoComponent);

        f("TrackPropertyEditor", &PropertyEditorManager::addTrackComponent);
        f("ControlPropertyEditor", &PropertyEditorManager::addTrackComponent);
        f("TrackDisplayPropertyEditor", &PropertyEditorManager::addTrackComponent);

    }

    void CorePlugin::checkLastRun() {
        auto settings = RuntimeInterface::settings();
        settings->setValue("lastInitializationAbortedFlag", false);
        bool lastRunWarningSuppressed = QApplication::arguments().contains(kNoWarningLastRun);
        if (settings->value("lastRunTerminatedAbnormally").toBool() && !lastRunWarningSuppressed) {
            qCWarning(lcCorePlugin) << "Last run terminated abnormally";
            SVS::MessageBox::warning(RuntimeInterface::qmlEngine(), nullptr, tr("Last run terminated abnormally"), tr("%1 did not exit normally during its last run.\n\nTo check for unsaved files, please go to Recovery Files.").arg(QApplication::applicationDisplayName()));
        }
        settings->setValue("lastRunTerminatedAbnormally", true);
        RuntimeInterface::addExitCallback([](int exitCode) {
            if (exitCode == 0) {
                RuntimeInterface::settings()->setValue("lastRunTerminatedAbnormally", false);
            }
        });
    }

    void CorePlugin::checkPlugins() {
        auto args = QApplication::arguments();
        if (auto i = args.indexOf(kFatalWithoutPlugins); i != -1) {
            QList<QString> pluginIdList;
            for (auto j = i + 1; j < args.size(); ++j) {
                if (args[j].startsWith('-'))
                    break;
                pluginIdList.append(args[j]);
            }
            QList<QString> requiredPluginsWithError;
            for (auto plugin : ExtensionSystem::PluginManager::plugins()) {
                if (pluginIdList.contains(plugin->name())) {
                    pluginIdList.removeAll(plugin->name());
                    if (plugin->hasError()) {
                        requiredPluginsWithError.append(plugin->name());
                    }
                }
            }
            if (!pluginIdList.isEmpty()) {
                qFatal() << "Missing plugins specified by" << kFatalWithoutPlugins << ":" << pluginIdList;
            }
            if (!requiredPluginsWithError.isEmpty()) {
                qFatal() << "Plugins specified by" << kFatalWithoutPlugins << "have errors:" << requiredPluginsWithError;
            }
        }

        QList<ExtensionSystem::PluginSpec *> errorPlugins;
        std::ranges::copy(
            ExtensionSystem::PluginManager::plugins() | std::views::filter([](auto *plugin) {
                return plugin->hasError();
            }),
            std::back_inserter(errorPlugins)
        );
        bool isPluginErrorSuppressed = QApplication::arguments().contains(kNoWarningPluginError);
        if (!errorPlugins.isEmpty() && !isPluginErrorSuppressed) {
            static auto errorMessage = tr("Errors occurred while loading some plugins:\n\n%1\n\nPlease go to Plugins to see more details.");
            QStringList pluginErrorMessages;
            std::ranges::transform(errorPlugins, std::back_inserter(pluginErrorMessages), [](auto plugin) {
                return tr("%1 (%2)").arg(plugin->displayName(), plugin->name());
            });
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), nullptr, tr("Plugin Error"), errorMessage.arg(pluginErrorMessages.join("\n")));
        }
    }

}
