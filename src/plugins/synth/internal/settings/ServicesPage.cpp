#include "ServicesPage.h"

#include <QAbstractItemModel>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/internal/ServiceConfigurationModel.h>
#include <synth/internal/SynthService.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcServicesPage, "diffscope.synth.servicespage")

    ServicesPage::ServicesPage(SynthService *service, QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Services"), parent),
          m_service(service ? service : SynthService::instance()),
          m_model(new ServiceConfigurationModel(this)) {
        setTitle(tr("Services"));
        setDescription(tr("Configure DSSP service instances"));
        connect(m_model, &ServiceConfigurationModel::edited, this, &Core::ISettingPage::markDirty);
    }

    ServicesPage::~ServicesPage() {
        delete m_widget;
    }

    QString ServicesPage::sortKeyword() const {
        return QStringLiteral("02Services");
    }

    bool ServicesPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *ServicesPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcServicesPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("ServicesPage"));
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

    void ServicesPage::beginSetting() {
        qCInfo(lcServicesPage) << "Beginning setting";
        widget();
        setErrorMessage({});
        m_model->setConfigurations(m_service->serviceConfigurations());
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool ServicesPage::accept() {
        const auto configurations = m_model->configurations();
        for (qsizetype i = 0; i < configurations.size(); ++i) {
            QStringList errors;
            if (!configurations.at(i).validate(&errors)) {
                setErrorMessage(tr("Service %1: %2")
                                    .arg(i + 1)
                                    .arg(errors.join(QStringLiteral("; "))));
                return false;
            }
        }

        QString errorMessage;
        if (!m_service->replaceServiceConfigurations(configurations, &errorMessage)) {
            setErrorMessage(errorMessage.isEmpty() ? tr("Could not save service configurations.") : errorMessage);
            return false;
        }
        setErrorMessage({});
        return Core::ISettingPage::accept();
    }

    void ServicesPage::endSetting() {
        if (m_widget)
            m_widget->setProperty("started", false);
        setErrorMessage({});
        Core::ISettingPage::endSetting();
    }

    QAbstractItemModel *ServicesPage::configurationModel() const {
        return m_model;
    }

    QString ServicesPage::errorMessage() const {
        return m_errorMessage;
    }

    bool ServicesPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        if (!matcher)
            return false;
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

    void ServicesPage::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

}
