// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H

#include <CoreApi/windowinterface.h>

namespace sflow {
    class RangeIndicatorInteractionController;
}

namespace SynthVisualizer::Internal {

    class SynthesisPieceModel;

    class SynthesisPieceTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(SynthesisPieceModel *pieceModel READ pieceModel CONSTANT)
        Q_PROPERTY(sflow::RangeIndicatorInteractionController *rangeIndicatorInteractionController READ rangeIndicatorInteractionController CONSTANT)

    public:
        explicit SynthesisPieceTrackAddOn(QObject *parent = nullptr);
        ~SynthesisPieceTrackAddOn() override;

        SynthesisPieceModel *pieceModel() const;
        sflow::RangeIndicatorInteractionController *rangeIndicatorInteractionController() const;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        SynthesisPieceModel *m_pieceModel{};
        sflow::RangeIndicatorInteractionController *m_rangeIndicatorInteractionController{};
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H
