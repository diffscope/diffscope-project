// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZERSETTINGS_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZERSETTINGS_H

#include <QObject>
#include <qqmlintegration.h>

class QJSEngine;
class QQmlEngine;

namespace SynthVisualizer::Internal {

    class SynthVisualizerPlugin;

    class SynthesisVisualizerSettings : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(bool showPhonemeWaveforms READ showPhonemeWaveforms NOTIFY showPhonemeWaveformsChanged)
        Q_PROPERTY(bool showClipWaveforms READ showClipWaveforms NOTIFY showClipWaveformsChanged)

    public:
        ~SynthesisVisualizerSettings() override;

        static SynthesisVisualizerSettings *instance();
        static SynthesisVisualizerSettings *create(QQmlEngine *engine, QJSEngine *scriptEngine);

        bool showPhonemeWaveforms() const;
        bool showClipWaveforms() const;
        void setOptions(bool showPhonemeWaveforms, bool showClipWaveforms);

    Q_SIGNALS:
        void showPhonemeWaveformsChanged();
        void showClipWaveformsChanged();

    private:
        explicit SynthesisVisualizerSettings(QObject *parent);

        static SynthesisVisualizerSettings *m_instance;
        bool m_showPhonemeWaveforms;
        bool m_showClipWaveforms;

        friend class SynthVisualizerPlugin;
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZERSETTINGS_H
