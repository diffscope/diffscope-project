// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERBANDMODEL_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERBANDMODEL_H

#include <QAbstractListModel>

#include <equalizereffectsunit/internal/EqualizerParameters.h>

namespace EqualizerEffectsUnit::Internal {

    class EqualizerBandModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum Role {
            FrequencyHzRole = Qt::UserRole + 1,
            GainDbRole,
            QRole,
            TypeRole,
            EnabledRole,
            SoloRole,
        };

        explicit EqualizerBandModel(QObject *parent = nullptr);
        ~EqualizerBandModel() override;

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        const EqualizerBandList &bands() const;
        const EqualizerBand *bandAt(int index) const;
        void setBands(const EqualizerBandList &bands);
        void insertBand(int index, const EqualizerBand &band);
        void removeBand(int index);
        bool updateBand(int index, const EqualizerBand &band);

    private:
        EqualizerBandList m_bands;
    };

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERBANDMODEL_H
