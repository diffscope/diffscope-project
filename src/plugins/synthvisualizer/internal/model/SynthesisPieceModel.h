// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H
#define DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPair>
#include <QPointer>
#include <qqmlintegration.h>

namespace Core {
    class ProjectWindowInterface;
}

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
        QML_ELEMENT
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle WRITE setWindowHandle NOTIFY windowHandleChanged)
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
            AudioFilePathRole,
            PieceRole,
            DiagnosticFilePathRole,
        };

        explicit SynthesisPieceModel(QObject *parent = nullptr);
        explicit SynthesisPieceModel(Synth::ProjectSynthesisContext *context, QObject *parent = nullptr);
        ~SynthesisPieceModel() override;

        Core::ProjectWindowInterface *windowHandle() const;
        void setWindowHandle(Core::ProjectWindowInterface *windowHandle);

        dspx::SingingClip *singingClip() const;
        void setSingingClip(dspx::SingingClip *clip);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE bool cancelPieceTask(QObject *piece);
        Q_INVOKABLE void resynthesizePiece(QObject *piece, int fromType, bool readCache, bool writeCache);

    Q_SIGNALS:
        void windowHandleChanged();
        void singingClipChanged();

    private:
        void setSynthesisContext(Synth::ProjectSynthesisContext *context);
        void rebuild();
        void updatePiece(Synth::SynthesisPiece *piece);
        void updateClipPosition();
        QString statusText(const Synth::SynthesisPiece *piece) const;
        QString taskTypeText(Synth::SynthesisTaskType type) const;
        QPair<double, double> absoluteRange(const Synth::SynthesisPiece *piece) const;
        void disconnectPieceSignals();
        void disconnectClipSignals();

        QPointer<Core::ProjectWindowInterface> m_windowHandle;
        QPointer<Synth::ProjectSynthesisContext> m_context;
        QPointer<dspx::SingingClip> m_singingClip;
        QList<QPointer<Synth::SynthesisPiece>> m_pieces;
        QMetaObject::Connection m_windowHandleConnection;
        QList<QMetaObject::Connection> m_contextConnections;
        QList<QMetaObject::Connection> m_pieceConnections;
        QList<QMetaObject::Connection> m_clipConnections;
    };

}

#endif // DIFFSCOPE_SYNTH_VISUALIZER_SYNTHESISPIECEMODEL_H
