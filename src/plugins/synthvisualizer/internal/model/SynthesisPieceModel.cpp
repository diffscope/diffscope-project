// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisPieceModel.h"

#include <algorithm>
#include <utility>

#include <QFileInfo>
#include <QSet>
#include <QVariant>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <ScopicFlowCore/RangeIndicatorViewModel.h>
#include <ScopicFlowCore/RangeSequenceViewModel.h>

#include <dspxmodelORM/SingingClip.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <synth/ProjectSynthesisContext.h>
#include <synth/SynthesisPiece.h>

namespace SynthVisualizer::Internal {

    SynthesisPieceModel::SynthesisPieceModel(QObject *parent)
        : QAbstractListModel(parent) {
        m_rangeIndicatorSequenceViewModel =
            new sflow::RangeSequenceViewModel(this, "position", "length");
    }

    SynthesisPieceModel::SynthesisPieceModel(Synth::ProjectSynthesisContext *context, QObject *parent)
        : SynthesisPieceModel(parent) {
        setSynthesisContext(context);
    }

    SynthesisPieceModel::~SynthesisPieceModel() {
        clearRangeIndicators();
        disconnectPieceSignals();
        disconnectClipSignals();
        disconnect(m_windowHandleConnection);
        for (const auto &connection : std::as_const(m_contextConnections)) {
            disconnect(connection);
        }
    }

    Core::ProjectWindowInterface *SynthesisPieceModel::windowHandle() const {
        return m_windowHandle;
    }

    void SynthesisPieceModel::setWindowHandle(Core::ProjectWindowInterface *windowHandle) {
        if (m_windowHandle == windowHandle) {
            return;
        }
        disconnect(m_windowHandleConnection);
        m_windowHandle = windowHandle;
        if (m_windowHandle) {
            m_windowHandleConnection = connect(m_windowHandle, &QObject::destroyed, this, [this] {
                m_windowHandle = nullptr;
                m_windowHandleConnection = {};
                setSynthesisContext(nullptr);
                Q_EMIT windowHandleChanged();
            });
        } else {
            m_windowHandleConnection = {};
        }
        setSynthesisContext(Synth::ProjectSynthesisContext::of(windowHandle));
        Q_EMIT windowHandleChanged();
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

    sflow::RangeSequenceViewModel *SynthesisPieceModel::rangeIndicatorSequenceViewModel() const {
        return m_rangeIndicatorSequenceViewModel;
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
            case AudioFilePathRole:
                return piece->audioFilePath();
            case PieceRole:
                return QVariant::fromValue(static_cast<QObject *>(piece));
            case DiagnosticFilePathRole: {
                const auto path = piece->diagnosticFilePath();
                return path.isEmpty() || !QFileInfo::exists(path) ? QString() : path;
            }
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
            {AudioFilePathRole, QByteArrayLiteral("audioFilePath")},
            {PieceRole, QByteArrayLiteral("piece")},
            {DiagnosticFilePathRole, QByteArrayLiteral("diagnosticFilePath")},
        };
    }

    bool SynthesisPieceModel::cancelPieceTask(QObject *pieceObject) {
        auto piece = qobject_cast<Synth::SynthesisPiece *>(pieceObject);
        if (!piece || !m_context)
            return false;
        return m_context->cancelPieceTask(piece);
    }

    void SynthesisPieceModel::resynthesizePiece(QObject *pieceObject, int fromType, bool readCache, bool writeCache) {
        auto piece = qobject_cast<Synth::SynthesisPiece *>(pieceObject);
        if (!piece || !m_context)
            return;
        if (fromType < static_cast<int>(Synth::SynthesisTaskType::Pronunciation) ||
            fromType > static_cast<int>(Synth::SynthesisTaskType::Audio))
            return;
        m_context->resynthesizePiece(
            piece, static_cast<Synth::SynthesisTaskType>(fromType), readCache, writeCache
        );
    }

    QObject *SynthesisPieceModel::pieceForRangeIndicator(
        sflow::RangeIndicatorViewModel *viewItem) const {
        return m_pieceMap.value(viewItem);
    }

    bool SynthesisPieceModel::isPieceTaskActive(QObject *pieceObject) const {
        auto piece = qobject_cast<Synth::SynthesisPiece *>(pieceObject);
        return piece && (piece->state() == Synth::SynthesisPiece::Queued ||
                         piece->state() == Synth::SynthesisPiece::Synthesizing);
    }

    QString SynthesisPieceModel::verifiedDiagnosticFilePath(QObject *pieceObject) const {
        auto piece = qobject_cast<Synth::SynthesisPiece *>(pieceObject);
        if (!piece) {
            return {};
        }
        const auto path = piece->diagnosticFilePath();
        return path.isEmpty() || !QFileInfo::exists(path) ? QString() : path;
    }

    void SynthesisPieceModel::setSynthesisContext(Synth::ProjectSynthesisContext *context) {
        if (m_context == context) {
            return;
        }
        for (const auto &connection : std::as_const(m_contextConnections)) {
            disconnect(connection);
        }
        m_contextConnections.clear();
        m_context = context;
        if (m_context) {
            m_contextConnections.append(connect(
                m_context, &Synth::ProjectSynthesisContext::piecesChanged,
                this, &SynthesisPieceModel::rebuild
            ));
            m_contextConnections.append(connect(
                m_context, &Synth::ProjectSynthesisContext::pieceChanged,
                this, &SynthesisPieceModel::updatePiece
            ));
            m_contextConnections.append(connect(m_context, &QObject::destroyed, this, [this] {
                m_context = nullptr;
                m_contextConnections.clear();
                rebuild();
            }));
        }
        rebuild();
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

        reconcileRangeIndicators();

        for (auto piecePointer : std::as_const(m_pieces)) {
            auto piece = piecePointer.data();
            if (!piece) {
                continue;
            }
            m_pieceConnections.append(connect(piece, &Synth::SynthesisPiece::rangeChanged,
                                              this, [this, piece] { updatePiece(piece); }));
            m_pieceConnections.append(connect(piece, &Synth::SynthesisPiece::stateChanged,
                                              this, [this, piece] { updatePiece(piece); }));
            m_pieceConnections.append(connect(piece, &Synth::SynthesisPiece::audioFileChanged,
                                              this, [this, piece] { updatePiece(piece); }));
        }
    }

    void SynthesisPieceModel::updatePiece(Synth::SynthesisPiece *piece) {
        if (!piece) {
            return;
        }
        updateRangeIndicator(piece);
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
                AudioFilePathRole,
                PieceRole,
                DiagnosticFilePathRole,
            });
            return;
        }
    }

    void SynthesisPieceModel::updateClipPosition() {
        for (auto piece : m_rangeIndicatorViewItemMap.keys()) {
            updateRangeIndicator(piece);
        }
        if (m_pieces.isEmpty()) {
            return;
        }
        Q_EMIT dataChanged(index(0), index(m_pieces.size() - 1), {
            AbsolutePositionRole,
        });
    }

    void SynthesisPieceModel::reconcileRangeIndicators() {
        QSet<Synth::SynthesisPiece *> livePieces;
        for (const auto piecePointer : std::as_const(m_pieces)) {
            if (auto piece = piecePointer.data()) {
                livePieces.insert(piece);
            }
        }

        for (auto piece : m_rangeIndicatorViewItemMap.keys()) {
            if (livePieces.contains(piece)) {
                continue;
            }
            auto viewItem = m_rangeIndicatorViewItemMap.take(piece);
            m_pieceMap.remove(viewItem);
            m_rangeIndicatorSequenceViewModel->removeItem(viewItem);
            viewItem->deleteLater();
        }

        for (auto piece : std::as_const(livePieces)) {
            auto viewItem = m_rangeIndicatorViewItemMap.value(piece);
            if (!viewItem) {
                viewItem = new sflow::RangeIndicatorViewModel(this);
                m_rangeIndicatorViewItemMap.insert(piece, viewItem);
                m_pieceMap.insert(viewItem, piece);
                updateRangeIndicator(piece);
                m_rangeIndicatorSequenceViewModel->insertItem(viewItem);
            } else {
                updateRangeIndicator(piece);
            }
        }
    }

    void SynthesisPieceModel::updateRangeIndicator(Synth::SynthesisPiece *piece) {
        auto viewItem = m_rangeIndicatorViewItemMap.value(piece);
        if (!viewItem || !m_singingClip) {
            return;
        }
        viewItem->setPosition(qRound(m_singingClip->start() + piece->position()));
        viewItem->setLength(qRound(piece->length()));
        viewItem->setContent(statusText(piece));
        switch (piece->state()) {
            case Synth::SynthesisPiece::Ready:
                viewItem->setType(SVS::SVSCraft::CT_Accent);
                break;
            case Synth::SynthesisPiece::Failed:
                viewItem->setType(SVS::SVSCraft::CT_Error);
                break;
            case Synth::SynthesisPiece::Idle:
            case Synth::SynthesisPiece::Stale:
            case Synth::SynthesisPiece::Queued:
            case Synth::SynthesisPiece::Synthesizing:
                viewItem->setType(SVS::SVSCraft::CT_Normal);
                break;
        }
    }

    void SynthesisPieceModel::clearRangeIndicators() {
        const auto viewItems = m_pieceMap.keys();
        for (auto viewItem : viewItems) {
            m_rangeIndicatorSequenceViewModel->removeItem(viewItem);
            delete viewItem;
        }
        m_rangeIndicatorViewItemMap.clear();
        m_pieceMap.clear();
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
