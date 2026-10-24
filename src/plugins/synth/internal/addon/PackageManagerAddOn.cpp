// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageManagerAddOn.h"

#include <initializer_list>
#include <memory>
#include <utility>

#include <QByteArray>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QStandardItem>
#include <QTimer>
#include <QUrl>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <uishell/USDef.h>
#include <coreplugin/ActionWindowInterfaceBase.h>

#include <synth/internal/DsspServerManager.h>
#include <synth/internal/PackageInstallModel.h>
#include <synth/internal/PackageManagerSettings.h>
#include <synth/internal/PackageManagerUtils.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerAddOn, "diffscope.synth.packagemanageraddon")

    namespace {

        QStandardItem *createItem(std::initializer_list<std::pair<int, QVariant>> roles) {
            auto item = new QStandardItem();
            for (const auto &[role, value] : roles) {
                item->setData(value, role);
            }
            return item;
        }

        QStandardItem *createSubtree(const QString &name) {
            return createItem({
                {UIShell::USDef::PR_NameRole, name},
            });
        }

        void addPackageSubtrees(QStandardItem *packageItem) {
            packageItem->setChild(UIShell::USDef::PI_Dependencies, createSubtree(Synth::Internal::PackageManagerAddOn::tr("Dependencies")));
            packageItem->setChild(UIShell::USDef::PI_OtherContributions, createSubtree(Synth::Internal::PackageManagerAddOn::tr("Other Contributions")));
            packageItem->setChild(UIShell::USDef::PI_Singers, createSubtree(Synth::Internal::PackageManagerAddOn::tr("Singers")));
        }

        QString resourcePath(const QJsonValue &value, const QString &directory) {
            const auto path = localizedPackageText(value);
            if (path.isEmpty()) {
                return {};
            }
            const auto url = QUrl(path);
            if (url.isLocalFile())
                return url.toLocalFile();
            if (url.isValid() && url.scheme().size() > 1) {
                return path;
            }
            return QDir::toNativeSeparators(QDir(directory).absoluteFilePath(path));
        }

        QString imageSourcePath(const QJsonValue &value, const QString &directory) {
            const auto path = resourcePath(value, directory);
            if (path.isEmpty())
                return {};
            const auto url = QUrl(path);
            return url.isValid() && url.scheme().size() > 1 ? path : QUrl::fromLocalFile(path).toString();
        }

        QString dspmTarget(const QString &id, const QString &version) {
            return id + QStringLiteral("=") + version;
        }

        struct RemovalResult {
            bool ok{};
            QStringList targets;
            QString errorMessage;
        };

        RemovalResult parseRemovalResult(const CommandResult &command, bool dryRun) {
            RemovalResult result;
            const auto mutation = parseDspmMutationResult(command, QStringLiteral("remove"), dryRun);
            if (!mutation.ok) {
                result.errorMessage = mutation.errorMessage;
                return result;
            }
            const auto actions = mutation.data.toObject().value(QStringLiteral("actions"));
            const auto invalidResult = [&] {
                result.ok = false;
                result.errorMessage = PackageManagerAddOn::tr("The package manager returned an invalid uninstall result.");
                return result;
            };
            static const QRegularExpression idPattern(QStringLiteral("\\A[A-Za-z0-9_-]+(?:/[A-Za-z0-9_-]+)*\\z"));
            static const QRegularExpression versionPattern(QStringLiteral("\\A(?:0|[1-9][0-9]*)(?:\\.(?:0|[1-9][0-9]*)){3}\\z"));
            QSet<QString> seen;
            for (const auto &value : actions.toArray()) {
                const auto action = value.toObject();
                const auto operation = action.value(QStringLiteral("action")).toString();
                const auto package = action.value(QStringLiteral("package")).toObject();
                const auto id = package.value(QStringLiteral("id")).toString();
                const auto version = package.value(QStringLiteral("version")).toString();
                const auto target = dspmTarget(id, version);
                if ((operation != QStringLiteral("remove") && !(dryRun && operation == QStringLiteral("cascade"))) || !idPattern.match(id).hasMatch() || !versionPattern.match(version).hasMatch() || seen.contains(target))
                    return invalidResult();
                seen.insert(target);
                result.targets.append(target);
            }
            result.ok = true;
            return result;
        }

        DspmResult runDspmJson(const QStringList &arguments) {
            const QPointer<DsspServerManager> server = Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>();
            const auto result = parseDspmQueryResult(runCommandLineTool(PackageManagerSettings::dspmPath(), arguments, PackageManagerSettings::timeoutSeconds(), [server] {
                return !server || server->isShuttingDown();
            }));
            if (!result.ok)
                qCWarning(lcPackageManagerAddOn) << "dspm query failed" << arguments << result.errorMessage;
            return result;
        }

        QStringList baseDspmArguments() {
            return {
                QStringLiteral("--json"),
                QStringLiteral("--packages-dir"),
                PackageManagerSettings::packageDir(),
            };
        }

        QStandardItem *createPackageItem(const QJsonObject &data, const QString &root) {
            const auto package = data.value(QStringLiteral("package")).toObject();
            const auto installation = data.value(QStringLiteral("installation")).toObject();
            const auto packagePath = QDir(root).absoluteFilePath(installation.value(QStringLiteral("directory")).toString());
            auto packageItem = createItem({
                {UIShell::USDef::PR_IdRole, package.value(QStringLiteral("id")).toString()},
                {UIShell::USDef::PR_VersionRole, package.value(QStringLiteral("version")).toString()},
                {UIShell::USDef::PR_PathRole, QDir::toNativeSeparators(packagePath)},
                {UIShell::USDef::PR_InstallationTimeRole, installation.value(QStringLiteral("installedAt")).toInteger()},
                {UIShell::USDef::PR_NameRole, localizedPackageText(package.value(QStringLiteral("name")))},
                {UIShell::USDef::PR_DescriptionRole, localizedPackageText(package.value(QStringLiteral("description")))},
                {UIShell::USDef::PR_VendorRole, localizedPackageText(package.value(QStringLiteral("vendor")))},
                {UIShell::USDef::PR_ReadmePathRole, localizedPackageText(package.value(QStringLiteral("readme")))},
                {UIShell::USDef::PR_LicensePathRole, localizedPackageText(package.value(QStringLiteral("license")))},
                {UIShell::USDef::PR_UrlRole, package.value(QStringLiteral("url")).toString()},
            });
            addPackageSubtrees(packageItem);

            auto dependencies = packageItem->child(UIShell::USDef::PI_Dependencies);
            for (const auto &dependencyValue : package.value(QStringLiteral("dependencies")).toArray()) {
                const auto dependency = dependencyValue.toObject();
                dependencies->appendRow(createItem({
                    {UIShell::USDef::PR_IdRole, dependency.value(QStringLiteral("id")).toString()},
                    {UIShell::USDef::PR_VersionRole, dependency.value(QStringLiteral("version")).toString()},
                }));
            }

            auto otherContributions = packageItem->child(UIShell::USDef::PI_OtherContributions);
            auto singers = packageItem->child(UIShell::USDef::PI_Singers);
            for (const auto &contributionValue : package.value(QStringLiteral("contributions")).toArray()) {
                const auto contribution = contributionValue.toObject();
                const auto category = contribution.value(QStringLiteral("category")).toString();
                const auto path = contribution.value(QStringLiteral("path")).toString();
                const auto contributionPath = path.isEmpty() ? QString{} : QDir(packagePath).absoluteFilePath(path);
                auto item = createItem({
                    {UIShell::USDef::PR_IdRole, contribution.value(QStringLiteral("id")).toString()},
                    {UIShell::USDef::PR_CategoryRole, category},
                    {UIShell::USDef::PR_PathRole, QDir::toNativeSeparators(contributionPath)},
                });
                if (category != QStringLiteral("singer")) {
                    otherContributions->appendRow(item);
                    continue;
                }
                const auto singer = contribution.value(QStringLiteral("singer")).toObject();
                const auto directory = QFileInfo(contributionPath).absolutePath();
                item->setData(localizedPackageText(contribution.value(QStringLiteral("name"))), UIShell::USDef::PR_NameRole);
                item->setData(contribution.value(QStringLiteral("interface")).toString(), UIShell::USDef::PR_InterfaceRole);
                const auto level = contribution.value(QStringLiteral("level"));
                item->setData(level.isDouble() ? QLocale().toString(level.toInteger()) : QString{}, UIShell::USDef::PR_LevelRole);
                item->setData(contribution.value(QStringLiteral("variant")).toString(), UIShell::USDef::PR_VariantRole);
                item->setData(contribution.value(QStringLiteral("metadataError")).toString(), UIShell::USDef::PR_MetadataErrorRole);
                item->setData(imageSourcePath(singer.value(QStringLiteral("avatar")), directory), UIShell::USDef::PR_AvatarPathRole);
                item->setData(imageSourcePath(singer.value(QStringLiteral("background")), directory), UIShell::USDef::PR_BackgroundPathRole);
                item->setData(resourcePath(singer.value(QStringLiteral("demoAudio")), directory), UIShell::USDef::PR_DemoAudioPathRole);
                singers->appendRow(item);
            }

            return packageItem;
        }

    }

    PackageManagerAddOn::PackageManagerAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    PackageManagerAddOn::~PackageManagerAddOn() = default;

    QStandardItemModel *PackageManagerAddOn::packageModel() {
        return &m_packageModel;
    }

    bool PackageManagerAddOn::refreshing() const {
        return m_refreshing;
    }

    bool PackageManagerAddOn::uninstalling() const {
        return m_uninstalling;
    }

    bool PackageManagerAddOn::installing() const {
        return m_installing;
    }

    QModelIndex PackageManagerAddOn::invalidIndex() const {
        return {};
    }

    void PackageManagerAddOn::openPackageManager(QWindow *parent) {
        if (m_window) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
            return;
        }

        qCInfo(lcPackageManagerAddOn) << "Opening package manager";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageManagerWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        auto window = qobject_cast<QWindow *>(object);
        if (!window) {
            qFatal("PackageManagerWindow in DiffScope.Synth is not QWindow");
        }
        std::unique_ptr<QObject> windowObject(object);
        m_window = window;
        window->setTransientParent(parent ? parent : windowHandle()->window());
        window->show();

        QEventLoop eventLoop;
        connect(window, SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
        m_window = nullptr;
    }

    void PackageManagerAddOn::refresh() {
        if (m_refreshing || m_uninstalling || m_installing || packageOperationActive()) {
            return;
        }

        QString errorMessage;
        if (!reloadPackageModel(&errorMessage))
            showError(tr("Failed to refresh packages:\n\n%1").arg(errorMessage));
    }

    bool PackageManagerAddOn::reloadPackageModel(QString *errorMessage, bool clearModel) {
        qCInfo(lcPackageManagerAddOn) << "Refreshing package model";
        setRefreshing(true);
        Q_EMIT refreshStarted();
        if (clearModel)
            m_packageModel.clear();

        QStandardItemModel model;
        const auto directory = PackageManagerSettings::packageDir();
        auto ok = populatePackageModel(&model, errorMessage);
        if (ok && directory != PackageManagerSettings::packageDir()) {
            ok = false;
            *errorMessage = tr("The package directory has changed. Refresh the package list before uninstalling packages.");
        }
        if (ok) {
            m_packageDirectory = directory;
            m_packageModel.clear();
            while (model.rowCount() > 0) {
                m_packageModel.appendRow(model.takeRow(0));
            }
            qCInfo(lcPackageManagerAddOn) << "Package model refreshed" << m_packageModel.rowCount();
        } else {
            qCCritical(lcPackageManagerAddOn) << "Failed to refresh package model" << *errorMessage;
        }

        setRefreshing(false);
        return ok;
    }

    void PackageManagerAddOn::uninstallPackage(int index) {
        if (m_refreshing || m_uninstalling || m_installing || packageOperationActive() || index < 0 || index >= m_packageModel.rowCount())
            return;
        if (m_packageDirectory != PackageManagerSettings::packageDir()) {
            showError(tr("The package directory has changed. Refresh the package list before uninstalling packages."));
            return;
        }
        const auto item = m_packageModel.item(index);
        const auto target = dspmTarget(item->data(UIShell::USDef::PR_IdRole).toString(), item->data(UIShell::USDef::PR_VersionRole).toString());
        setUninstalling(true);
        const auto guard = qScopeGuard([this] { setUninstalling(false); });

        const auto executablePath = PackageManagerSettings::dspmPath();
        const auto arguments = baseDspmArguments();
        const auto timeoutSeconds = PackageManagerSettings::timeoutSeconds();
        const auto packageLabel = [this](const QString &selector) {
            for (int row = 0; row < m_packageModel.rowCount(); ++row) {
                const auto item = m_packageModel.item(row);
                if (dspmTarget(item->data(UIShell::USDef::PR_IdRole).toString(), item->data(UIShell::USDef::PR_VersionRole).toString()) != selector)
                    continue;
                const auto name = item->data(UIShell::USDef::PR_NameRole).toString();
                return name.isEmpty() ? selector : tr("%1 (%2)").arg(name, selector);
            }
            return selector;
        };
        QQmlComponent progressComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageUninstallProgressDialog");
        if (progressComponent.isError())
            qFatal() << progressComponent.errorString();
        auto progressObject = progressComponent.createWithInitialProperties({
            {QStringLiteral("packages"), QStringList{packageLabel(target)}},
            {QStringLiteral("transientParent"), QVariant::fromValue(m_window.data())},
        });
        if (!progressObject)
            qFatal() << progressComponent.errorString();
        std::unique_ptr<QObject> progressDialog(progressObject);
        auto progressWindow = qobject_cast<QWindow *>(progressObject);
        if (!progressWindow)
            qFatal("PackageUninstallProgressDialog in DiffScope.Synth is not QWindow");

        const auto runRemoval = [&](const QStringList &targets, bool dryRun) {
            auto commandArguments = arguments;
            commandArguments.append(QStringLiteral("remove"));
            if (dryRun) {
                commandArguments.append(QStringLiteral("--dry-run"));
                commandArguments.append(QStringLiteral("--cascade"));
            }
            commandArguments.append(QStringLiteral("--"));
            commandArguments.append(targets);
            QByteArray pendingOutput;
            const auto outputReceived = [&](const QByteArray &output) {
                pendingOutput.append(output);
                qsizetype lineEnd;
                while ((lineEnd = pendingOutput.indexOf('\n')) >= 0) {
                    const auto line = pendingOutput.first(lineEnd);
                    pendingOutput.remove(0, lineEnd + 1);
                    const auto document = QJsonDocument::fromJson(line);
                    if (!document.isObject())
                        continue;
                    const auto record = document.object();
                    if (record.value(QStringLiteral("command")).toString() != QStringLiteral("remove") || record.value(QStringLiteral("event")).toString() != QStringLiteral("progress"))
                        continue;
                    const auto phase = record.value(QStringLiteral("data")).toObject().value(QStringLiteral("phase")).toString();
                    if (phase == QStringLiteral("resolve"))
                        progressDialog->setProperty("statusText", tr("Checking package dependencies..."));
                    else if (phase == QStringLiteral("commit"))
                        progressDialog->setProperty("statusText", tr("Uninstalling packages..."));
                }
            };
            const QPointer<DsspServerManager> server = Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>();
            return parseRemovalResult(runCommandLineTool(executablePath, commandArguments, timeoutSeconds, [server, dryRun] {
                return !server || server->isShuttingDown() || (!dryRun && !server->packageOperationActive());
            }, outputReceived), dryRun);
        };

        qCInfo(lcPackageManagerAddOn) << "Preparing package removal" << target;
        progressWindow->show();
        auto plan = runRemoval({target}, true);
        progressWindow->hide();
        if (!plan.ok || !plan.targets.contains(target)) {
            const auto message = plan.ok ? tr("The package manager returned an invalid uninstall result.") : plan.errorMessage;
            showError(tr("Failed to prepare package uninstallation:\n\n%1").arg(message));
            return;
        }
        plan.targets.removeAll(target);
        plan.targets.prepend(target);
        QStringList labels;
        for (const auto &selector : std::as_const(plan.targets))
            labels.append(packageLabel(selector));

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageUninstallDialog");
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("packageName"), packageLabel(target)},
            {QStringLiteral("packages"), labels},
            {QStringLiteral("transientParent"), QVariant::fromValue(m_window.data())},
        });
        if (!object)
            qFatal() << component.errorString();
        std::unique_ptr<QObject> dialog(object);
        if (SVS::MessageBox::customExec(dialog.get()).toInt() != SVS::SVSCraft::Yes)
            return;

        if (arguments.at(2) != PackageManagerSettings::packageDir() || executablePath != PackageManagerSettings::dspmPath()) {
            showError(tr("The package manager settings have changed. Retry the operation."));
            return;
        }
        const QPointer<DsspServerManager> server = Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>();
        QString stopError;
        if (!server->beginPackageOperation(m_window, &stopError)) {
            if (!stopError.isEmpty())
                showError(stopError);
            return;
        }
        const auto operationGuard = qScopeGuard([server] {
            if (server)
                server->endPackageOperation();
        });
        qCInfo(lcPackageManagerAddOn) << "Removing packages" << plan.targets;
        progressDialog->setProperty("packages", labels);
        progressDialog->setProperty("statusText", tr("Uninstalling packages..."));
        progressWindow->show();
        auto result = runRemoval(plan.targets, false);
        const QSet<QString> expected(plan.targets.cbegin(), plan.targets.cend());
        const QSet<QString> removed(result.targets.cbegin(), result.targets.cend());
        if (result.ok && removed != expected) {
            result.ok = false;
            result.errorMessage = tr("The package manager returned an invalid uninstall result.");
        }
        QString refreshError;
        progressDialog->setProperty("statusText", tr("Refreshing package information..."));
        const auto refreshed = reloadPackageModel(&refreshError, true);
        progressWindow->hide();
        if (server)
            server->endPackageOperation();
        QString message;
        if (!result.ok)
            message = tr("Failed to uninstall packages:\n\n%1").arg(result.errorMessage);
        if (!refreshed) {
            if (!message.isEmpty())
                message.append(QStringLiteral("\n\n"));
            message.append(tr("Failed to refresh packages:\n\n%1").arg(refreshError));
        }
        if (!message.isEmpty())
            showError(message);
    }

    void PackageManagerAddOn::installPackages(const QList<QUrl> &files) {
        if (m_refreshing || m_uninstalling || m_installing || packageOperationActive() || files.isEmpty())
            return;
        setInstalling(true);
        const auto guard = qScopeGuard([this] { setInstalling(false); });
        PackageInstallModel model;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageInstallDialog");
        if (component.isError())
            qFatal() << component.errorString();
        auto object = component.createWithInitialProperties({
            {QStringLiteral("installModel"), QVariant::fromValue(&model)},
            {QStringLiteral("transientParent"), QVariant::fromValue(m_window.data())},
        });
        if (!object)
            qFatal() << component.errorString();
        std::unique_ptr<QObject> dialog(object);
        QTimer::singleShot(0, &model, [&model, files] { model.addFiles(files); });
        SVS::MessageBox::customExec(dialog.get());
        if (model.attemptedInstallation()) {
            QString errorMessage;
            if (!reloadPackageModel(&errorMessage, true))
                showError(tr("Failed to refresh packages:\n\n%1").arg(errorMessage));
        }
    }

    void PackageManagerAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        windowInterface->addObject(this);
        auto server = Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>();
        Q_ASSERT(server);
        connect(server, &DsspServerManager::stateChanged, this, &PackageManagerAddOn::packageOperationActiveChanged);
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "PackageManagerActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void PackageManagerAddOn::extensionsInitialized() {
    }

    bool PackageManagerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    bool PackageManagerAddOn::populatePackageModel(QStandardItemModel *model, QString *errorMessage) const {
        const auto arguments = baseDspmArguments();
        auto listArguments = arguments;
        listArguments.append(QStringLiteral("list"));

        const auto listRoot = runDspmJson(listArguments);
        if (!listRoot.ok) {
            *errorMessage = listRoot.errorMessage;
            return false;
        }

        if (listRoot.command != QStringLiteral("list") || !listRoot.data.isArray()) {
            *errorMessage = tr("The command returned invalid package data.");
            return false;
        }
        const auto packages = listRoot.data.toArray();
        for (const auto &packageValue : packages) {
            const auto package = packageValue.toObject();
            const auto id = package.value(QStringLiteral("id")).toString();
            const auto version = package.value(QStringLiteral("version")).toString();
            if (id.isEmpty() || version.isEmpty()) {
                *errorMessage = tr("The command returned invalid package data.");
                return false;
            }

            auto infoArguments = arguments;
            infoArguments.append(QStringLiteral("info"));
            infoArguments.append(QStringLiteral("--installed"));
            infoArguments.append(QStringLiteral("--"));
            infoArguments.append(dspmTarget(id, version));

            const auto infoRoot = runDspmJson(infoArguments);
            if (!infoRoot.ok) {
                *errorMessage = tr("Failed to get package information for %1:\n%2").arg(dspmTarget(id, version), infoRoot.errorMessage);
                return false;
            }
            const auto data = infoRoot.data.toObject();
            const auto manifest = data.value(QStringLiteral("package")).toObject();
            const auto installation = data.value(QStringLiteral("installation")).toObject();
            static const QRegularExpression directoryPattern(QStringLiteral("^[0-9a-f]{64}$"));
            if (infoRoot.command != QStringLiteral("info") ||
                manifest.value(QStringLiteral("id")).toString() != id || manifest.value(QStringLiteral("version")).toString() != version ||
                !manifest.value(QStringLiteral("dependencies")).isArray() || !manifest.value(QStringLiteral("contributions")).isArray() ||
                !directoryPattern.match(installation.value(QStringLiteral("directory")).toString()).hasMatch() || !installation.value(QStringLiteral("installedAt")).isDouble()) {
                *errorMessage = tr("The command returned invalid package data.");
                return false;
            }
            model->appendRow(createPackageItem(data, arguments.at(2)));
        }
        return true;
    }

    void PackageManagerAddOn::setRefreshing(bool refreshing) {
        if (m_refreshing == refreshing)
            return;
        m_refreshing = refreshing;
        Q_EMIT refreshingChanged();
    }

    bool PackageManagerAddOn::packageOperationActive() const {
        return Core::RuntimeInterface::instance()->getFirstObject<DsspServerManager>()->packageOperationActive();
    }

    void PackageManagerAddOn::setUninstalling(bool uninstalling) {
        if (m_uninstalling == uninstalling)
            return;
        m_uninstalling = uninstalling;
        Q_EMIT uninstallingChanged();
    }

    void PackageManagerAddOn::setInstalling(bool installing) {
        if (m_installing == installing)
            return;
        m_installing = installing;
        Q_EMIT installingChanged();
    }

    void PackageManagerAddOn::showError(const QString &message) const {
        SVS::MessageBox::critical(
            Core::RuntimeInterface::qmlEngine(),
            m_window,
            tr("Package Manager"),
            message
        );
    }

}
