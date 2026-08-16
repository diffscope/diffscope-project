// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ArchitecturePage.h"

#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QSettings>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synth/internal/ArchitectureExtraModel.h>

namespace Synth::Internal {

    ArchitecturePage::ArchitecturePage(QObject *parent)
        : Core::ISettingPage(QStringLiteral("org.diffscope.synth.Architecture"), parent),
          m_model(new ArchitectureExtraModel(this)) {
        setTitle(tr("Architecture"));
        setDescription(tr("Configure architecture-specific synthesis options"));
        connect(m_model, &ArchitectureExtraModel::edited, this, &Core::ISettingPage::markDirty);
    }

    ArchitecturePage::~ArchitecturePage() {
        delete m_widget;
    }

    QString ArchitecturePage::sortKeyword() const {
        return QStringLiteral("01Architecture");
    }

    bool ArchitecturePage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QObject *ArchitecturePage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("ArchitecturePage"));
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

    void ArchitecturePage::beginSetting() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(
            settings->value(QStringLiteral("architectureExtras")).toByteArray(), &error
        );
        settings->endGroup();
        m_model->setEntries(error.error == QJsonParseError::NoError && document.isObject() ? document.object() : QJsonObject{});
        setErrorMessage({});
        Core::ISettingPage::beginSetting();
    }

    bool ArchitecturePage::accept() {
        QJsonObject entries;
        QString errorMessage;
        if (!m_model->entries(&entries, &errorMessage)) {
            setErrorMessage(errorMessage);
            return false;
        }

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synth"));
        settings->setValue(QStringLiteral("architectureExtras"), QJsonDocument(entries).toJson(QJsonDocument::Compact));
        settings->endGroup();
        setErrorMessage({});
        return Core::ISettingPage::accept();
    }

    void ArchitecturePage::endSetting() {
        setErrorMessage({});
        Core::ISettingPage::endSetting();
    }

    QAbstractItemModel *ArchitecturePage::configurationModel() const {
        return m_model;
    }

    QString ArchitecturePage::errorMessage() const {
        return m_errorMessage;
    }

    bool ArchitecturePage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result{};
        return matcher && QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word) && result;
    }

    void ArchitecturePage::setErrorMessage(const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }

}

#include "moc_ArchitecturePage.cpp"
