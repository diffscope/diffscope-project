// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H

#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QVariantList>

#include <CoreApi/windowinterface.h>

class QAbstractItemModel;

namespace dspx {
    class SelectionModel;
    class Track;
}

namespace EffectsUnitManager::Internal {

    class EffectsContext;

    class EffectsPanelAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *effectsModel READ effectsModel NOTIFY selectionContextChanged)
        Q_PROPERTY(QString selectionMessage READ selectionMessage NOTIFY selectionContextChanged)
        Q_PROPERTY(bool hasEffectsContext READ hasEffectsContext NOTIFY selectionContextChanged)
        Q_PROPERTY(bool trackTabVisible READ trackTabVisible NOTIFY selectionContextChanged)
        Q_PROPERTY(QString trackTabText READ trackTabText NOTIFY selectionContextChanged)
        Q_PROPERTY(int activeTab READ activeTab WRITE setActiveTab NOTIFY selectionContextChanged)
        Q_PROPERTY(bool readingFilterConflict READ readingFilterConflict NOTIFY selectionContextChanged)
        Q_PROPERTY(QString readingFilterConflictMessage READ readingFilterConflictMessage NOTIFY selectionContextChanged)
        Q_PROPERTY(QVariantList availableEffects READ availableEffects NOTIFY availableEffectsChanged)

    public:
        enum Tab {
            TrackTab,
            MasterTab,
        };
        Q_ENUM(Tab)

        explicit EffectsPanelAddOn(QObject *parent = nullptr);
        ~EffectsPanelAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QAbstractItemModel *effectsModel() const;
        QString selectionMessage() const;
        bool hasEffectsContext() const;
        bool trackTabVisible() const;
        QString trackTabText() const;
        int activeTab() const;
        void setActiveTab(int activeTab);
        bool readingFilterConflict() const;
        QString readingFilterConflictMessage() const;
        QVariantList availableEffects() const;

        Q_INVOKABLE bool addEffect(const QString &id);
        Q_INVOKABLE bool removeEffect(int row);
        Q_INVOKABLE bool setEffectEnabled(int row, bool enabled);
        Q_INVOKABLE bool moveEffect(int row, int offset);
        Q_INVOKABLE bool resetEffect(int row);
        Q_INVOKABLE void setExpanded(int row, bool expanded);

    Q_SIGNALS:
        void selectionContextChanged();
        void availableEffectsChanged();

    private:
        EffectsContext *activeContext() const;
        void createMasterContext();
        void createTrackContext(dspx::Track *track);
        void refreshSelection();
        void refreshAvailableEffects();
        void setTrackSelection(bool tabVisible,
                               dspx::Track *track,
                               EffectsContext *context,
                               const QString &message);
        void clearAssociationConnections();

        dspx::SelectionModel *m_selectionModel{};
        QHash<dspx::Track *, QPointer<EffectsContext>> m_trackContexts;
        QPointer<EffectsContext> m_masterContext;
        QPointer<EffectsContext> m_trackContext;
        QPointer<dspx::Track> m_selectedTrack;
        QString m_trackSelectionMessage;
        QString m_masterSelectionMessage;
        QVariantList m_availableEffects;
        QList<QMetaObject::Connection> m_associationConnections;
        bool m_trackTabVisible{};
        int m_activeTab{MasterTab};
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H
