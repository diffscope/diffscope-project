// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ITEMSELECTORLISTMODEL_H
#define DIFFSCOPE_COREPLUGIN_ITEMSELECTORLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QVariant>

namespace SVS {
    class MusicTimeline;
}

namespace dspx {
    class AnchorNodeSequence;
    class ClipSequence;
    class DynamicMixingAnchorSequence;
    class KeySignatureSequence;
    class LabelSequence;
    class NoteSequence;
    class ParameterMap;
    class SelectionModel;
    class TempoSequence;
    class TrackList;
}

namespace Core {
    class ParameterInfoProvider;
    class ProjectWindowInterface;
}

namespace Core::Internal {

    class KeySignatureAtSpecifiedPositionHelper;

    enum class NodeKind {
        RootTracks,
        RootLabels,
        RootTempos,
        RootKeySignatures,
        Track,
        Clip,
        SingingNotes,
        SingingParameters,
        SingingVoiceBlending,
        Note,
        Parameter,
        EditedAnchors,
        TransformAnchors,
        Anchor,
        DynamicAnchor,
        Label,
        Tempo,
        KeySignature,
    };

    enum class ListKind {
        Root,
        Tracks,
        Clips,
        SingingBranches,
        Notes,
        Parameters,
        AnchorBranches,
        Anchors,
        DynamicAnchors,
        Labels,
        Tempos,
        KeySignatures,
    };

    struct ItemSelectorEntry {
        NodeKind kind{};
        QPointer<QObject> object;
        QString key;

        bool matches(const ItemSelectorEntry &other) const;
    };

    class ItemSelectorListModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum Role {
            DescriptionRole = Qt::UserRole + 1,
            ObjectRole,
            NodeKindRole,
            HasChildrenRole,
        };

        ItemSelectorListModel(ListKind kind, QObject *context,
                              ProjectWindowInterface *windowInterface,
                              QObject *parent = nullptr);
        ~ItemSelectorListModel() override;

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        Qt::CheckState checkStateSummary() const;
        bool hasSelectableItems() const;
        bool isSelectableList() const;
        void setAllSelected(bool selected);

        const ItemSelectorEntry *entryAt(int row) const;
        int rowFor(NodeKind kind, QObject *object = nullptr,
                   const QString &key = {}) const;
        ListKind listKind() const;
        QObject *context() const;

    Q_SIGNALS:
        void checkStateSummaryChanged();

    private:
        bool isSelectable(const ItemSelectorEntry &entry) const;
        bool isSelected(const ItemSelectorEntry &entry) const;
        bool hasChildren(const ItemSelectorEntry &entry) const;
        QString title(const ItemSelectorEntry &entry, int row) const;
        QString description(const ItemSelectorEntry &entry) const;
        QList<ItemSelectorEntry> sourceEntries() const;

        void bindContainer();
        void bindTrackList(dspx::TrackList *list);
        void bindClipSequence(dspx::ClipSequence *sequence);
        void bindNoteSequence(dspx::NoteSequence *sequence);
        void bindParameterMap(dspx::ParameterMap *map);
        void bindAnchorSequence(dspx::AnchorNodeSequence *sequence);
        void bindDynamicSequence(dspx::DynamicMixingAnchorSequence *sequence);
        void bindLabelSequence(dspx::LabelSequence *sequence);
        void bindTempoSequence(dspx::TempoSequence *sequence);
        void bindKeySignatureSequence(dspx::KeySignatureSequence *sequence);
        void switchDynamicContainer();
        void synchronize();
        void attachObject(const ItemSelectorEntry &entry);
        void detachObject(QObject *object);
        void refreshObject(QObject *object, bool reorder);
        int rowForObject(QObject *object) const;
        void refreshAll(const QList<int> &roles);

        QString parameterArchitectureId() const;
        void bindParameterArchitecture();
        void updateParameterProviders();
        void rebuildAnchorProvider();
        void bindAnchorArchitecture();
        void clearObjectBindings();
        void clearMetadataConnections();
        void clearBindings();

        ListKind m_kind;
        QPointer<QObject> m_context;
        dspx::SelectionModel *m_selectionModel;
        SVS::MusicTimeline *m_musicTimeline;
        QList<ItemSelectorEntry> m_entries;
        QList<QMetaObject::Connection> m_ownerConnections;
        QList<QMetaObject::Connection> m_containerConnections;
        QList<QMetaObject::Connection> m_metadataConnections;
        QHash<QObject *, QList<QMetaObject::Connection>> m_objectConnections;
        QHash<QObject *, QObject *> m_singerResolvers;
        QHash<QObject *, ParameterInfoProvider *> m_parameterProviders;
        QObject *m_contextSingerResolver = nullptr;
        ParameterInfoProvider *m_anchorProvider = nullptr;
        KeySignatureAtSpecifiedPositionHelper *m_keySignatureHelper = nullptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ITEMSELECTORLISTMODEL_H
