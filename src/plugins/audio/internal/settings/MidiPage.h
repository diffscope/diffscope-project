// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIPAGE_H
#define DIFFSCOPE_AUDIO_MIDIPAGE_H

#include <CoreApi/isettingpage.h>

namespace Audio::Internal {

    class MidiPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit MidiPage(QObject *parent = nullptr);
        ~MidiPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIPAGE_H
