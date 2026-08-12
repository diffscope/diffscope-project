#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H

#include <CoreApi/windowinterface.h>

class QAbstractItemModel;

namespace SynthVisualizer::Internal {

    class SynthesisPieceModel;

    class SynthesisPieceTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *pieceModel READ pieceModel CONSTANT)

    public:
        explicit SynthesisPieceTrackAddOn(QObject *parent = nullptr);
        ~SynthesisPieceTrackAddOn() override;

        QAbstractItemModel *pieceModel() const;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        SynthesisPieceModel *m_pieceModel{};
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECETRACKADDON_H
