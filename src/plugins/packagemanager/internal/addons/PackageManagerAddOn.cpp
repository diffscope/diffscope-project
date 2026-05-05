#include "PackageManagerAddOn.h"

#include <memory>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QStandardItem>
#include <QUrl>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ActionWindowInterfaceBase.h>

#include <packagemanager/internal/PackageManagerSettings.h>
#include <packagemanager/internal/PackageManagerUtils.h>

#include <uishell/USDef.h>

namespace PackageManager {

    Q_STATIC_LOGGING_CATEGORY(lcPackageManagerAddOn, "diffscope.packagemanager.packagemanageraddon")

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
            packageItem->setChild(UIShell::USDef::PI_Dependencies, createSubtree(PackageManagerAddOn::tr("Dependencies")));
            packageItem->setChild(UIShell::USDef::PI_Inferences, createSubtree(PackageManagerAddOn::tr("Inferences")));
            packageItem->setChild(UIShell::USDef::PI_Singers, createSubtree(PackageManagerAddOn::tr("Singers")));
        }

        QStandardItem *addSinger(QStandardItem *singers, QStandardItem *singer) {
            singer->setChild(UIShell::USDef::PI_SingerImports, createSubtree(PackageManagerAddOn::tr("Imports")));
            singer->setChild(UIShell::USDef::PI_SingerDemoAudioList, createSubtree(PackageManagerAddOn::tr("Demo Audio List")));
            singers->appendRow(singer);
            return singer;
        }

        QString packageDirOptionName() {
            return QStringLiteral("--packages-dir");
        }

        QString jsonText(const QJsonValue &value) {
            if (value.isString()) {
                return value.toString();
            }
            if (value.isDouble()) {
                return QString::number(value.toDouble());
            }
            if (value.isObject()) {
                const auto object = value.toObject();
                if (object.contains(QStringLiteral("_"))) {
                    return object.value(QStringLiteral("_")).toString();
                }
                if (object.contains(QStringLiteral("default"))) {
                    return object.value(QStringLiteral("default")).toString();
                }
                for (auto it = object.begin(); it != object.end(); ++it) {
                    if (it.value().isString()) {
                        return it.value().toString();
                    }
                }
            }
            return {};
        }

        QString imageSourcePath(const QJsonValue &value) {
            const auto path = jsonText(value);
            if (path.isEmpty()) {
                return {};
            }
            const auto url = QUrl(path);
            if (url.isValid() && url.scheme().size() > 1) {
                return path;
            }
            return QUrl::fromLocalFile(path).toString();
        }

        QString dspmTarget(const QString &id, const QString &version) {
            if (version.isEmpty()) {
                return id;
            }
            return id + QStringLiteral("@") + version;
        }

        QString dspmJsonError(const QJsonObject &root) {
            const auto error = root.value(QStringLiteral("error")).toObject();
            const auto message = error.value(QStringLiteral("message")).toString();
            const auto code = error.value(QStringLiteral("code")).toString();
            if (!message.isEmpty() && !code.isEmpty()) {
                return QStringLiteral("%1: %2").arg(code, message);
            }
            return message;
        }

        bool runDspmJson(const QStringList &arguments, QJsonObject *root, QString *errorMessage) {
            const auto result = runCommandLineTool(PackageManagerSettings::dspmPath(), arguments, PackageManagerSettings::timeoutSeconds());
            if (!result.ok) {
                qCWarning(lcPackageManagerAddOn) << "dspm failed" << arguments << result.errorMessage;
                if (errorMessage) {
                    *errorMessage = result.errorMessage;
                }
                return false;
            }

            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(result.stdOut, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                qCWarning(lcPackageManagerAddOn) << "Failed to parse dspm JSON" << parseError.errorString();
                if (errorMessage) {
                    *errorMessage = PackageManagerAddOn::tr("The command did not return valid JSON.");
                }
                return false;
            }

            *root = document.object();
            if (!root->value(QStringLiteral("ok")).toBool()) {
                auto message = dspmJsonError(*root);
                if (message.isEmpty()) {
                    message = PackageManagerAddOn::tr("The command reported an error.");
                }
                qCWarning(lcPackageManagerAddOn) << "dspm JSON error" << message;
                if (errorMessage) {
                    *errorMessage = message;
                }
                return false;
            }
            return true;
        }

        QStringList baseDspmArguments() {
            return {
                QStringLiteral("--json"),
                packageDirOptionName(),
                PackageManagerSettings::packageDir(),
            };
        }

        QStandardItem *createPackageItem(const QJsonObject &data) {
            const auto package = data.value(QStringLiteral("package")).toObject();
            const auto installation = data.value(QStringLiteral("installation")).toObject();
            auto packageItem = createItem({
                {UIShell::USDef::PR_IdRole, package.value(QStringLiteral("id")).toString()},
                {UIShell::USDef::PR_VersionRole, package.value(QStringLiteral("version")).toString()},
                {UIShell::USDef::PR_PathRole, installation.value(QStringLiteral("path")).toString()},
                {UIShell::USDef::PR_InstallationTimeRole, installation.value(QStringLiteral("installedAt")).toString()},
                {UIShell::USDef::PR_NameRole, jsonText(package.value(QStringLiteral("name")))},
                {UIShell::USDef::PR_DescriptionRole, jsonText(package.value(QStringLiteral("description")))},
                {UIShell::USDef::PR_VendorRole, jsonText(package.value(QStringLiteral("vendor")))},
                {UIShell::USDef::PR_ReadmePathRole, jsonText(package.value(QStringLiteral("readme")))},
                {UIShell::USDef::PR_LicensePathRole, jsonText(package.value(QStringLiteral("license")))},
                {UIShell::USDef::PR_UrlRole, package.value(QStringLiteral("url")).toString()},
            });
            addPackageSubtrees(packageItem);

            auto dependencies = packageItem->child(UIShell::USDef::PI_Dependencies);
            for (const auto &dependencyValue : data.value(QStringLiteral("dependencies")).toArray()) {
                const auto dependency = dependencyValue.toObject();
                dependencies->appendRow(createItem({
                    {UIShell::USDef::PR_IdRole, dependency.value(QStringLiteral("id")).toString()},
                    {UIShell::USDef::PR_VersionRole, dependency.value(QStringLiteral("version")).toString()},
                    {UIShell::USDef::PR_NameRole, jsonText(dependency.value(QStringLiteral("name")))},
                }));
            }

            auto inferences = packageItem->child(UIShell::USDef::PI_Inferences);
            for (const auto &inferenceValue : data.value(QStringLiteral("inferences")).toArray()) {
                const auto inference = inferenceValue.toObject();
                inferences->appendRow(createItem({
                    {UIShell::USDef::PR_IdRole, inference.value(QStringLiteral("id")).toString()},
                    {UIShell::USDef::PR_ClassNameRole, inference.value(QStringLiteral("class")).toString()},
                    {UIShell::USDef::PR_NameRole, jsonText(inference.value(QStringLiteral("name")))},
                    {UIShell::USDef::PR_PathRole, inference.value(QStringLiteral("path")).toString()},
                }));
            }

            auto singers = packageItem->child(UIShell::USDef::PI_Singers);
            for (const auto &singerValue : data.value(QStringLiteral("singers")).toArray()) {
                const auto singer = singerValue.toObject();
                auto singerItem = addSinger(singers, createItem({
                    {UIShell::USDef::PR_IdRole, singer.value(QStringLiteral("id")).toString()},
                    {UIShell::USDef::PR_ClassNameRole, singer.value(QStringLiteral("class")).toString()},
                    {UIShell::USDef::PR_NameRole, jsonText(singer.value(QStringLiteral("name")))},
                    {UIShell::USDef::PR_PathRole, singer.value(QStringLiteral("path")).toString()},
                    {UIShell::USDef::PR_AvatarPathRole, imageSourcePath(singer.value(QStringLiteral("avatar")))},
                    {UIShell::USDef::PR_BackgroundPathRole, imageSourcePath(singer.value(QStringLiteral("background")))},
                }));

                auto imports = singerItem->child(UIShell::USDef::PI_SingerImports);
                for (const auto &importValue : singer.value(QStringLiteral("imports")).toArray()) {
                    const auto import = importValue.toObject();
                    imports->appendRow(createItem({
                        {UIShell::USDef::PR_IdRole, import.value(QStringLiteral("id")).toString()},
                        {UIShell::USDef::PR_VersionRole, import.value(QStringLiteral("version")).toString()},
                        {UIShell::USDef::PR_NameRole, jsonText(import.value(QStringLiteral("name")))},
                        {UIShell::USDef::PR_ImportInferenceIdRole, import.value(QStringLiteral("inferenceId")).toString()},
                    }));
                }

                auto demoAudioList = singerItem->child(UIShell::USDef::PI_SingerDemoAudioList);
                for (const auto &demoAudioValue : singer.value(QStringLiteral("demoAudio")).toArray()) {
                    const auto demoAudio = demoAudioValue.toObject();
                    demoAudioList->appendRow(createItem({
                        {UIShell::USDef::PR_NameRole, jsonText(demoAudio.value(QStringLiteral("name")))},
                        {UIShell::USDef::PR_PathRole, jsonText(demoAudio.value(QStringLiteral("audio")))},
                    }));
                }
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

    QModelIndex PackageManagerAddOn::invalidIndex() const {
        return {};
    }

    void PackageManagerAddOn::openPackageManager() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        if (m_window) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
            return;
        }

        qCInfo(lcPackageManagerAddOn) << "Opening package manager";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.PackageManager", "PackageManagerWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> window(qobject_cast<QWindow *>(component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}})));
        Q_ASSERT(window);
        m_window = window.get();
        window->setTransientParent(windowInterface->window());
        window->show();

        QEventLoop eventLoop;
        connect(window.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
        m_window = nullptr;
    }

    void PackageManagerAddOn::refresh() {
        if (m_refreshing) {
            return;
        }

        qCInfo(lcPackageManagerAddOn) << "Refreshing package model";
        Q_EMIT refreshStarted();
        setRefreshing(true);

        QStandardItemModel model;
        QString errorMessage;
        const auto ok = populatePackageModel(&model, &errorMessage);
        if (ok) {
            m_packageModel.clear();
            while (model.rowCount() > 0) {
                m_packageModel.appendRow(model.takeRow(0));
            }
            qCInfo(lcPackageManagerAddOn) << "Package model refreshed" << m_packageModel.rowCount();
        } else {
            qCCritical(lcPackageManagerAddOn) << "Failed to refresh package model" << errorMessage;
        }

        setRefreshing(false);
        if (!ok) {
            showError(errorMessage);
        }
    }

    void PackageManagerAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.PackageManager", "PackageManagerActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void PackageManagerAddOn::extensionsInitialized() {
    }

    bool PackageManagerAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    bool PackageManagerAddOn::populatePackageModel(QStandardItemModel *model, QString *errorMessage) const {
        auto listArguments = baseDspmArguments();
        listArguments.append(QStringLiteral("list"));
        listArguments.append(QStringLiteral("--column"));
        listArguments.append(QStringLiteral("id,version,name,installed_at"));

        QJsonObject listRoot;
        if (!runDspmJson(listArguments, &listRoot, errorMessage)) {
            return false;
        }

        const auto packages = listRoot.value(QStringLiteral("data")).toObject().value(QStringLiteral("packages")).toArray();
        for (const auto &packageValue : packages) {
            const auto package = packageValue.toObject();
            const auto id = package.value(QStringLiteral("id")).toString();
            const auto version = package.value(QStringLiteral("version")).toString();
            if (id.isEmpty()) {
                continue;
            }

            auto infoArguments = baseDspmArguments();
            infoArguments.append(QStringLiteral("info"));
            infoArguments.append(dspmTarget(id, version));

            QJsonObject infoRoot;
            if (!runDspmJson(infoArguments, &infoRoot, errorMessage)) {
                if (errorMessage) {
                    *errorMessage = tr("Failed to get package information for %1:\n%2").arg(dspmTarget(id, version), *errorMessage);
                }
                return false;
            }
            model->appendRow(createPackageItem(infoRoot.value(QStringLiteral("data")).toObject()));
        }
        return true;
    }

    void PackageManagerAddOn::setRefreshing(bool refreshing) {
        if (m_refreshing == refreshing)
            return;
        m_refreshing = refreshing;
        Q_EMIT refreshingChanged();
    }

    void PackageManagerAddOn::showError(const QString &message) const {
        SVS::MessageBox::critical(
            Core::RuntimeInterface::qmlEngine(),
            m_window,
            tr("Package Manager"),
            tr("Failed to refresh packages:\n\n%1").arg(message)
        );
    }

}
