#include "SynthesisPieceModel.h"

#include <algorithm>
#include <utility>

#include <dspxmodelORM/SingingClip.h>

#include <synth/ProjectSynthesisContext.h>
#include <synth/SynthesisPiece.h>

namespace SynthVisualizer::Internal {

    SynthesisPieceModel::SynthesisPieceModel(Synth::ProjectSynthesisContext *context, QObject *parent)
        : QAbstractListModel(parent), m_context(context) {
        if (!m_context) {
            return;
        }
        connect(m_context, &Synth::ProjectSynthesisContext::piecesChanged,
                this, &SynthesisPieceModel::rebuild);
        connect(m_context, &Synth::ProjectSynthesisContext::pieceChanged,
                this, &SynthesisPieceModel::updatePiece);
        connect(m_context, &QObject::destroyed, this, [this] {
            m_context = nullptr;
            rebuild();
        });
    }

    SynthesisPieceModel::~SynthesisPieceModel() {
        disconnectPieceSignals();
        disconnectClipSignals();
    }

    dspx::SingingClip *SynthesisPieceModel::singingClip() const {
        return m_singingClip;
    }

    void SynthesisPieceModel::setSingingClip(dspx::SingingClip *clip) {
        if (m_singingClip == clip) {
            return;
        }
        disconnectClipSignals();
        m_singingClip = clip;
        if (m_singingClip) {
            m_clipConnections.append(connect(m_singingClip, &dspx::SingingClip::startChanged,
                                             this, [this] { updateClipPosition(); }));
            m_clipConnections.append(connect(m_singingClip, &QObject::destroyed, this, [this] {
                m_singingClip = nullptr;
                rebuild();
                Q_EMIT singingClipChanged();
            }));
        }
        rebuild();
        Q_EMIT singingClipChanged();
    }

    int SynthesisPieceModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_pieces.size();
    }

    QVariant SynthesisPieceModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_pieces.size()) {
            return {};
        }
        auto piece = m_pieces.at(index.row()).data();
        if (!piece) {
            return {};
        }
        const auto [position, duration] = absoluteRange(piece);
        switch (role) {
            case AbsolutePositionRole:
                return position;
            case DurationRole:
                return duration;
            case StatusTextRole:
                return statusText(piece);
            case ErrorMessageRole:
                return piece->errorMessage();
            case ActiveRole:
                return piece->state() == Synth::SynthesisPiece::Queued ||
                       piece->state() == Synth::SynthesisPiece::Synthesizing;
            case ReadyRole:
                return piece->state() == Synth::SynthesisPiece::Ready;
            case FailedRole:
                return piece->state() == Synth::SynthesisPiece::Failed;
            default:
                return {};
        }
    }

    QHash<int, QByteArray> SynthesisPieceModel::roleNames() const {
        return {
            {AbsolutePositionRole, QByteArrayLiteral("absolutePosition")},
            {DurationRole, QByteArrayLiteral("duration")},
            {StatusTextRole, QByteArrayLiteral("statusText")},
            {ErrorMessageRole, QByteArrayLiteral("errorMessage")},
            {ActiveRole, QByteArrayLiteral("active")},
            {ReadyRole, QByteArrayLiteral("ready")},
            {FailedRole, QByteArrayLiteral("failed")},
        };
    }

    void SynthesisPieceModel::rebuild() {
        disconnectPieceSignals();
        QList<Synth::SynthesisPiece *> pieces;
        if (m_context && m_singingClip) {
            pieces = m_context->piecesForClip(m_singingClip);
            std::sort(pieces.begin(), pieces.end(), [](const auto left, const auto right) {
                if (!left || !right) {
                    return left != nullptr;
                }
                return left->position() < right->position();
            });
        }

        beginResetModel();
        m_pieces.clear();
        m_pieces.reserve(pieces.size());
        for (auto piece : std::as_const(pieces)) {
            if (piece) {
                m_pieces.append(piece);
            }
        }
        endResetModel();

        for (auto piecePointer : std::as_const(m_pieces)) {
            auto piece = piecePointer.data();
            if (!piece) {
                continue;
            }
            m_pieceConnections.append(connect(piece, &Synth::SynthesisPiece::rangeChanged,
                                              this, [this, piece] { updatePiece(piece); }));
            m_pieceConnections.append(connect(piece, &Synth::SynthesisPiece::stateChanged,
                                              this, [this, piece] { updatePiece(piece); }));
        }
    }

    void SynthesisPieceModel::updatePiece(Synth::SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        for (int row = 0; row < m_pieces.size(); ++row) {
            if (m_pieces.at(row) != piece) {
                continue;
            }
            const auto modelIndex = index(row);
            Q_EMIT dataChanged(modelIndex, modelIndex, {
                AbsolutePositionRole,
                DurationRole,
                StatusTextRole,
                ErrorMessageRole,
                ActiveRole,
                ReadyRole,
                FailedRole,
            });
            return;
        }
    }

    void SynthesisPieceModel::updateClipPosition() {
        if (m_pieces.isEmpty()) {
            return;
        }
        Q_EMIT dataChanged(index(0), index(m_pieces.size() - 1), {
            AbsolutePositionRole,
        });
    }

    QString SynthesisPieceModel::statusText(const Synth::SynthesisPiece *piece) const {
        switch (piece->state()) {
            case Synth::SynthesisPiece::Idle:
                return tr("Not synthesized");
            case Synth::SynthesisPiece::Stale:
                return tr("Waiting");
            case Synth::SynthesisPiece::Queued:
                return tr("Queued: %1").arg(taskTypeText(piece->currentTaskType()));
            case Synth::SynthesisPiece::Synthesizing:
                return tr("Synthesizing: %1").arg(taskTypeText(piece->currentTaskType()));
            case Synth::SynthesisPiece::Ready:
                return tr("Ready");
            case Synth::SynthesisPiece::Failed:
                return tr("Failed");
        }
        return {};
    }

    QString SynthesisPieceModel::taskTypeText(Synth::SynthesisTaskType type) const {
        switch (type) {
            case Synth::SynthesisTaskType::Pronunciation:
                return tr("Pronunciation");
            case Synth::SynthesisTaskType::Phoneme:
                return tr("Phoneme");
            case Synth::SynthesisTaskType::Duration:
                return tr("Duration");
            case Synth::SynthesisTaskType::Parameter:
                return tr("Parameter");
            case Synth::SynthesisTaskType::Audio:
                return tr("Audio");
        }
        return {};
    }

    QPair<double, double> SynthesisPieceModel::absoluteRange(const Synth::SynthesisPiece *piece) const {
        if (!m_singingClip || !piece) {
            return {};
        }
        return {m_singingClip->start() + piece->position(), piece->length()};
    }

    void SynthesisPieceModel::disconnectPieceSignals() {
        for (const auto &connection : std::as_const(m_pieceConnections)) {
            disconnect(connection);
        }
        m_pieceConnections.clear();
    }

    void SynthesisPieceModel::disconnectClipSignals() {
        for (const auto &connection : std::as_const(m_clipConnections)) {
            disconnect(connection);
        }
        m_clipConnections.clear();
    }

}

#include "moc_SynthesisPieceModel.cpp"
