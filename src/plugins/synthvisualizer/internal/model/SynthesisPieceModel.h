#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPair>
#include <QPointer>

namespace dspx {
    class SingingClip;
}

namespace Synth {
    class ProjectSynthesisContext;
    class SynthesisPiece;
    enum class SynthesisTaskType;
}

namespace SynthVisualizer::Internal {

    class SynthesisPieceModel : public QAbstractListModel {
        Q_OBJECT
        Q_PROPERTY(dspx::SingingClip *singingClip READ singingClip WRITE setSingingClip NOTIFY singingClipChanged)

    public:
        enum Role {
            AbsolutePositionRole = Qt::UserRole + 1,
            DurationRole,
            StatusTextRole,
            ErrorMessageRole,
            ActiveRole,
            ReadyRole,
            FailedRole,
        };

        explicit SynthesisPieceModel(Synth::ProjectSynthesisContext *context, QObject *parent = nullptr);
        ~SynthesisPieceModel() override;

        dspx::SingingClip *singingClip() const;
        void setSingingClip(dspx::SingingClip *clip);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void singingClipChanged();

    private:
        void rebuild();
        void updatePiece(Synth::SynthesisPiece *piece);
        void updateClipPosition();
        QString statusText(const Synth::SynthesisPiece *piece) const;
        QString taskTypeText(Synth::SynthesisTaskType type) const;
        QPair<double, double> absoluteRange(const Synth::SynthesisPiece *piece) const;
        void disconnectPieceSignals();
        void disconnectClipSignals();

        QPointer<Synth::ProjectSynthesisContext> m_context;
        QPointer<dspx::SingingClip> m_singingClip;
        QList<QPointer<Synth::SynthesisPiece>> m_pieces;
        QList<QMetaObject::Connection> m_pieceConnections;
        QList<QMetaObject::Connection> m_clipConnections;
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H
