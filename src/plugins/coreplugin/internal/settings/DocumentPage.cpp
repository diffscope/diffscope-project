// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DocumentPage.h"

#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/internal/BehaviorPreference.h>

namespace Core::Internal {

    DocumentPage::DocumentPage(QObject *parent) : ISettingPage("org.diffscope.core.Document", parent) {
        setTitle(tr("Document"));
        setDescription(tr("Configure note and parameter editing behaviors"));
    }

    DocumentPage::~DocumentPage() {
        delete m_widget;
    }

    bool DocumentPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString DocumentPage::sortKeyword() const {
        return QStringLiteral("Document");
    }

    QObject *DocumentPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "DocumentPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        if (!m_widget) {
            qFatal() << component.errorString();
        }
        m_widget->setParent(this);
        return m_widget;
    }

    void DocumentPage::beginSetting() {
        widget();
        m_widget->setProperty("fallbackLyricText", BehaviorPreference::fallbackLyricText());
        m_widget->setProperty("skipParameterResamplingCheck", BehaviorPreference::skipParameterResamplingCheck());
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool DocumentPage::accept() {
        BehaviorPreference::setFallbackLyricText(m_widget->property("fallbackLyricText").toString());
        BehaviorPreference::setSkipParameterResamplingCheck(m_widget->property("skipParameterResamplingCheck").toBool());
        BehaviorPreference::instance()->save();
        return ISettingPage::accept();
    }

    void DocumentPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool DocumentPage::widgetMatches(const QString &word) {
        widget();
        auto *matcher = m_widget->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

}
