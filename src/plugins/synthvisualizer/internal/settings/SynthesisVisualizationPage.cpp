// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisVisualizationPage.h"

#include <QMetaObject>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <synthvisualizer/internal/SynthesisVisualizerSettings.h>

namespace SynthVisualizer::Internal {

    SynthesisVisualizationPage::SynthesisVisualizationPage(QObject *parent)
        : ISettingPage(QStringLiteral("org.diffscope.synthvisualizer.Visualization"), parent) {
        setTitle(tr("Synthesis Visualization"));
        setDescription(tr("Configure the display of synthesis waveforms"));
    }

    QString SynthesisVisualizationPage::sortKeyword() const {
        return QStringLiteral("Synthesis Visualization");
    }

    bool SynthesisVisualizationPage::matches(const QString &word) {
        if (ISettingPage::matches(word))
            return true;
        auto matcher = widget()->property("matcher").value<QObject *>();
        bool matches = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(matches), word);
        return matches;
    }

    QObject *SynthesisVisualizationPage::widget() {
        if (m_widget)
            return m_widget;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.SynthVisualizer", "SynthesisVisualizationPage");
        if (component.isError())
            qFatal() << component.errorString();
        m_widget = component.createWithInitialProperties({
            {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
        });
        if (!m_widget)
            qFatal() << component.errorString();
        m_widget->setParent(this);
        return m_widget;
    }

    void SynthesisVisualizationPage::beginSetting() {
        auto page = widget();
        const auto settings = SynthesisVisualizerSettings::instance();
        page->setProperty("showPhonemeWaveforms", settings->showPhonemeWaveforms());
        page->setProperty("showClipWaveforms", settings->showClipWaveforms());
        ISettingPage::beginSetting();
    }

    bool SynthesisVisualizationPage::accept() {
        SynthesisVisualizerSettings::instance()->setOptions(
            m_widget->property("showPhonemeWaveforms").toBool(),
            m_widget->property("showClipWaveforms").toBool()
        );
        return ISettingPage::accept();
    }

}

#include "moc_SynthesisVisualizationPage.cpp"
