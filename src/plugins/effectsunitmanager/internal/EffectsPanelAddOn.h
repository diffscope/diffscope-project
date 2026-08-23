// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H

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

    class TrackEffectsContext;

    class EffectsPanelAddOn final : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *effectsModel READ effectsModel NOTIFY selectionContextChanged)
        Q_PROPERTY(QString selectionMessage READ selectionMessage NOTIFY selectionContextChanged)
        Q_PROPERTY(bool hasTrack READ hasTrack NOTIFY selectionContextChanged)
        Q_PROPERTY(bool readingFilterConflict READ readingFilterConflict NOTIFY selectionContextChanged)
        Q_PROPERTY(QVariantList availableEffects READ availableEffects NOTIFY availableEffectsChanged)

    public:
        explicit EffectsPanelAddOn(QObject *parent = nullptr);
        ~EffectsPanelAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QAbstractItemModel *effectsModel() const;
        QString selectionMessage() const;
        bool hasTrack() const;
        bool readingFilterConflict() const;
        QVariantList availableEffects() const;

        Q_INVOKABLE bool addEffect(const QString &id);
        Q_INVOKABLE bool removeEffect(int row);
        Q_INVOKABLE bool setEffectEnabled(int row, bool enabled);
        Q_INVOKABLE bool moveEffect(int row, int offset);
        Q_INVOKABLE void setExpanded(int row, bool expanded);

    Q_SIGNALS:
        void selectionContextChanged();
        void availableEffectsChanged();

    private:
        void createTrackContext(dspx::Track *track);
        void refreshSelection();
        void refreshAvailableEffects();
        void setCurrentContext(TrackEffectsContext *context, const QString &message);
        void clearAssociationConnections();

        dspx::SelectionModel *m_selectionModel{};
        QPointer<TrackEffectsContext> m_currentContext;
        QString m_selectionMessage;
        QVariantList m_availableEffects;
        QList<QMetaObject::Connection> m_associationConnections;
        QMetaObject::Connection m_contextDestroyedConnection;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPANELADDON_H
