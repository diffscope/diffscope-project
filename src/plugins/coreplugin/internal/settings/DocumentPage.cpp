// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DocumentPage.h"

#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/SingerSettings.h>
#include <coreplugin/SourcesPicker.h>
#include <coreplugin/SourcesPickerModel.h>
#include <coreplugin/internal/BehaviorPreference.h>

namespace Core::Internal {

    DocumentPage::DocumentPage(QObject *parent)
        : ISettingPage("org.diffscope.core.Document", parent), m_defaultSingerModel(new SourcesPickerModel(this)) {
        setTitle(tr("Document"));
        setDescription(tr("Configure document editing behaviors"));
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
        m_widget = component.createWithInitialProperties({
            {"pageHandle", QVariant::fromValue(this)},
            {"defaultSingerModel", QVariant::fromValue(m_defaultSingerModel)},
        });
        if (!m_widget) {
            qFatal() << component.errorString();
        }
        m_widget->setParent(this);
        return m_widget;
    }

    void DocumentPage::beginSetting() {
        widget();
        m_defaultSingerModel->setSourcesSelection(SingerSettings::defaultSinger());
        m_widget->setProperty("fallbackLyricText", BehaviorPreference::fallbackLyricText());
        m_widget->setProperty("skipParameterResamplingCheck", BehaviorPreference::skipParameterResamplingCheck());
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool DocumentPage::accept() {
        if (!SingerSettings::setDefaultSinger(m_defaultSingerModel->sourcesSelection()))
            return false;
        BehaviorPreference::setFallbackLyricText(m_widget->property("fallbackLyricText").toString());
        BehaviorPreference::setSkipParameterResamplingCheck(m_widget->property("skipParameterResamplingCheck").toBool());
        BehaviorPreference::instance()->save();
        return ISettingPage::accept();
    }

    void DocumentPage::endSetting() {
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    void DocumentPage::editDefaultSinger(QQuickWindow *window) {
        const auto result = pickSources(m_defaultSingerModel->sourcesSelection(), window);
        if (result)
            m_defaultSingerModel->setSourcesSelection(*result);
    }

    bool DocumentPage::widgetMatches(const QString &word) {
        widget();
        auto *matcher = m_widget->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

}
