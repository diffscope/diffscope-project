// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_TRACKEFFECTSCONTEXT_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_TRACKEFFECTSCONTEXT_H

#include <memory>
#include <vector>

#include <QAbstractListModel>

namespace Audio {
    class TrackAudioContext;
}

namespace dspx {
    class AudioDSP;
    class AudioDSPList;
    class Track;
}

namespace EffectsUnitManager {
    class EffectsUnit;
}

namespace EffectsUnitManager::Internal {

    class EffectsChainFilter;

    class TrackEffectsContext final : public QAbstractListModel {
        Q_OBJECT
        Q_PROPERTY(dspx::Track *track READ track CONSTANT)
        Q_PROPERTY(bool readingFilterConflict READ readingFilterConflict CONSTANT)

    public:
        enum Role {
            IdRole = Qt::UserRole + 1,
            NameRole,
            EnabledRole,
            KnownRole,
            EditorRole,
            ExpandedRole,
            ErrorRole,
        };

        explicit TrackEffectsContext(Audio::TrackAudioContext *trackAudioContext);
        ~TrackEffectsContext() override;

        static TrackEffectsContext *of(dspx::Track *track);

        dspx::Track *track() const;
        bool readingFilterConflict() const;

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        bool addEffect(const QString &id);
        bool removeEffect(int row);
        bool setEffectEnabled(int row, bool enabled);
        bool moveEffect(int row, int offset);
        void setExpanded(int row, bool expanded);

    private:
        struct Entry;

        Entry *entryAt(int row) const;
        int indexOf(dspx::AudioDSP *item) const;
        void insertEntry(int index, dspx::AudioDSP *item);
        void removeEntry(int index);
        void recreateUnit(Entry &entry);
        void createUnit(Entry &entry, EffectsUnit *existingUnit = nullptr);
        void updateAudioChain();
        void handleUnitUpdated(dspx::AudioDSP *item);
        void restoreUnitState(Entry &entry);

        Audio::TrackAudioContext *m_trackAudioContext{};
        dspx::Track *m_track{};
        dspx::AudioDSPList *m_audioDSPList{};
        bool m_readingFilterConflict{};
        std::unique_ptr<EffectsChainFilter> m_filter;
        std::vector<std::unique_ptr<Entry>> m_entries;
        dspx::AudioDSP *m_pendingItem{};
        EffectsUnit *m_pendingUnit{};
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_TRACKEFFECTSCONTEXT_H
