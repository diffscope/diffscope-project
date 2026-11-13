// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZATIONPAGE_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZATIONPAGE_H

#include <CoreApi/isettingpage.h>

namespace SynthVisualizer::Internal {

    class SynthesisVisualizationPage : public Core::ISettingPage {
        Q_OBJECT

    public:
        explicit SynthesisVisualizationPage(QObject *parent = nullptr);

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;

    private:
        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISVISUALIZATIONPAGE_H
