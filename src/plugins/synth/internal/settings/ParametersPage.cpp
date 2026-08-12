#include "ParametersPage.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMap>
#include <QQmlComponent>
#include <QSet>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/SynthInterface.h>
#include <synth/internal/ParameterConfigurationModel.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcParametersPage, "diffscope.synth.parameterspage")

    static constexpr auto ParameterConfigurationFormat = "org.diffscope.synth.parameter-config";
    static constexpr int ParameterConfigurationFormatVersion = 1;

    static QString localFilePath(const QUrl &url) {
        if (url.isLocalFile())
            return url.toLocalFile();
        return url.scheme().isEmpty() ? url.toString() : QString{};
    }

    static QSet<QString> currentBuiltinParameterIds() {
        QSet<QString> result;
        const auto interface = SynthInterface::instance();
        if (interface) {
            for (const auto &configuration : interface->builtinParameterConfigurations())
                result.insert(configuration.id());
        }
        return result;
    }

    ParametersPage::ParametersPage(SynthService *service, QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Parameters"), parent),
          m_service(service ? service : SynthService::instance()),
          m_model(new ParameterConfigurationModel(this)) {
        setTitle(tr("Parameters"));
        setDescription(tr("Configure how DSSP parameters are displayed and mapped"));
        connect(m_model, &ParameterConfigurationModel::edited, this, &Core::ISettingPage::markDirty);
    }

    ParametersPage::~ParametersPage() {
        delete m_widget;
    }

    QString ParametersPage::sortKeyword() const {
        return QStringLiteral("03Parameters");
    }

    bool ParametersPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *ParametersPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcParametersPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("ParametersPage"));
        if (component.isError())
            qFatal() << component.errorString();
        m_widget = component.createWithInitialProperties({
            {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
            {QStringLiteral("configurationModel"), QVariant::fromValue(m_model)},
        });
        if (!m_widget)
            qFatal() << component.errorString();
        m_widget->setParent(this);
        return m_widget;
    }

    void ParametersPage::beginSetting() {
        qCInfo(lcParametersPage) << "Beginning setting";
        widget();
        setMessages({}, {});
        m_model->setConfigurations(m_service->allParameterConfigurations(),
                                   m_service->userParameterConfigurations());
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool ParametersPage::accept() {
        const auto configurations = m_model->userConfigurations();
        const auto builtinIds = currentBuiltinParameterIds();
        QSet<QString> ids;
        for (const auto &configuration : configurations) {
            if (builtinIds.contains(configuration.id())) {
                setMessages(tr("Parameter ID '%1' is provided by a read-only built-in configuration.")
                                .arg(configuration.id()));
                return false;
            }
            if (ids.contains(configuration.id())) {
                setMessages(tr("Parameter ID '%1' is duplicated.").arg(configuration.id()));
                return false;
            }
            ids.insert(configuration.id());
            QStringList errors;
            if (!configuration.validate(&errors)) {
                setMessages(tr("Parameter '%1': %2")
                                .arg(configuration.id(), errors.join(QStringLiteral("; "))));
                return false;
            }
        }

        QString errorMessage;
        if (!m_service->replaceUserParameterConfigurations(configurations, &errorMessage)) {
            setMessages(errorMessage.isEmpty() ? tr("Could not save parameter configurations.") : errorMessage);
            return false;
        }
        setMessages({}, {});
        return Core::ISettingPage::accept();
    }

    void ParametersPage::endSetting() {
        if (m_widget)
            m_widget->setProperty("started", false);
        setMessages({}, {});
        Core::ISettingPage::endSetting();
    }

    QAbstractItemModel *ParametersPage::configurationModel() const {
        return m_model;
    }

    QString ParametersPage::errorMessage() const {
        return m_errorMessage;
    }

    QString ParametersPage::statusMessage() const {
        return m_statusMessage;
    }

    bool ParametersPage::importFromFile(const QUrl &fileUrl) {
        const QString path = localFilePath(fileUrl);
        QFile file(path);
        if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) {
            setMessages(tr("Could not open the parameter configuration file."));
            return false;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setMessages(tr("Invalid JSON: %1").arg(parseError.errorString()));
            return false;
        }

        const auto root = document.object();
        if (root.value(QStringLiteral("format")).toString() != QLatin1String(ParameterConfigurationFormat)
            || root.value(QStringLiteral("version")).toInt(-1) != ParameterConfigurationFormatVersion
            || !root.value(QStringLiteral("parameters")).isArray()) {
            setMessages(tr("This is not a supported synthesis parameter configuration file."));
            return false;
        }

        QMap<QString, ParameterConfiguration> importedById;
        QStringList ignoredIds;
        const auto builtinIds = currentBuiltinParameterIds();
        const auto array = root.value(QStringLiteral("parameters")).toArray();
        for (qsizetype i = 0; i < array.size(); ++i) {
            if (!array.at(i).isObject()) {
                setMessages(tr("Parameter entry %1 is not an object.").arg(i + 1));
                return false;
            }
            const auto object = array.at(i).toObject();
            const QString id = object.value(QStringLiteral("id")).toString();
            if (id == QStringLiteral("pitch") || builtinIds.contains(id)) {
                ignoredIds.append(id);
                continue;
            }

            ParameterConfiguration configuration;
            QString errorMessage;
            if (!ParameterConfiguration::fromJson(object, &configuration, &errorMessage)) {
                setMessages(tr("Parameter entry %1: %2").arg(i + 1).arg(errorMessage));
                return false;
            }
            QStringList validationErrors;
            if (!configuration.validate(&validationErrors)) {
                setMessages(tr("Parameter '%1': %2")
                                .arg(configuration.id(), validationErrors.join(QStringLiteral("; "))));
                return false;
            }
            importedById.insert(configuration.id(), configuration);
        }

        m_model->mergeImported(importedById.values(), &ignoredIds);
        ignoredIds.removeDuplicates();
        const QString status = ignoredIds.isEmpty()
                                   ? tr("Imported %1 parameter configuration(s).").arg(importedById.size())
                                   : tr("Imported %1 parameter configuration(s); ignored reserved or built-in IDs: %2.")
                                         .arg(importedById.size())
                                         .arg(ignoredIds.join(QStringLiteral(", ")));
        setMessages({}, status);
        return true;
    }

    bool ParametersPage::exportToFile(const QUrl &fileUrl) {
        const QString path = localFilePath(fileUrl);
        QFile file(path);
        if (path.isEmpty() || !file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setMessages(tr("Could not create the parameter configuration file."));
            return false;
        }

        QJsonArray array;
        for (const auto &configuration : m_model->allConfigurations())
            array.append(configuration.toJson());
        const QJsonObject root{
            {QStringLiteral("format"), QLatin1String(ParameterConfigurationFormat)},
            {QStringLiteral("version"), ParameterConfigurationFormatVersion},
            {QStringLiteral("parameters"), array},
        };
        if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
            setMessages(tr("Could not write the parameter configuration file."));
            return false;
        }
        setMessages({}, tr("Exported %1 parameter configuration(s).").arg(array.size()));
        return true;
    }

    bool ParametersPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        if (!matcher)
            return false;
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

    void ParametersPage::setMessages(const QString &error, const QString &status) {
        if (m_errorMessage == error && m_statusMessage == status)
            return;
        m_errorMessage = error;
        m_statusMessage = status;
        Q_EMIT messageChanged();
    }

}
