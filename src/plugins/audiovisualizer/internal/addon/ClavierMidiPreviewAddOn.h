// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_VISUALIZER_CLAVIERMIDIPREVIEWADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_CLAVIERMIDIPREVIEWADDON_H

#include <QPointer>
#include <QtTypes>

#include <CoreApi/windowinterface.h>

namespace Audio {
    class MidiInputContext;
}

namespace AudioVisualizer::Internal {

    class ClavierMidiPreviewAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ClavierMidiPreviewAddOn(QObject *parent = nullptr);
        ~ClavierMidiPreviewAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

    private:
        QPointer<Audio::MidiInputContext> m_inputContext;
        const quint64 m_sourceId;
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_CLAVIERMIDIPREVIEWADDON_H
