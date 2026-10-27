// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <audio/internal/MidiInputSettingsHelper.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcMidiPage, "diffscope.audio.midipage")

    MidiPage::MidiPage(QObject *parent) : Core::ISettingPage(QStringLiteral("audio.Midi"), parent) {
        setTitle(tr("MIDI"));
        setDescription(tr("Configure MIDI input, preview sounds, and playback control"));
    }

    MidiPage::~MidiPage() {
        delete m_widget;
    }

    bool MidiPage::matches(const QString &word) {
        if (ISettingPage::matches(word))
            return true;
        auto matcher = widget()->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

    QString MidiPage::sortKeyword() const {
        return QStringLiteral("Midi");
    }

    QObject *MidiPage::widget() {
        if (!m_widget) {
            qCDebug(lcMidiPage) << "Creating widget";
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "MidiPage");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            m_widget = component.createWithInitialProperties({
                {QStringLiteral("pageHandle"), QVariant::fromValue(this)},
            });
            if (!m_widget) {
                qFatal() << component.errorString();
            }
            m_widget->setParent(this);
        }
        return m_widget;
    }

    void MidiPage::beginSetting() {
        qCInfo(lcMidiPage) << "Beginning setting";
        widget();
        m_widget->setProperty("helper", QVariant::fromValue(new MidiInputSettingsHelper(this)));
        ISettingPage::beginSetting();
    }

    bool MidiPage::accept() {
        qCInfo(lcMidiPage) << "Accepting";
        return ISettingPage::accept();
    }

    void MidiPage::endSetting() {
        qCInfo(lcMidiPage) << "Ending setting";
        auto helper = m_widget->property("helper").value<QObject *>();
        m_widget->setProperty("helper", QVariant::fromValue<MidiInputSettingsHelper *>(nullptr));
        delete helper;
        ISettingPage::endSetting();
    }

}
