// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerBandModel.h"

#include <QtGlobal>

namespace EqualizerEffectsUnit::Internal {

    namespace {

        bool valuesEqual(double left, double right) {
            return qFuzzyIsNull(left - right);
        }

        bool bandsEqual(const EqualizerBand &left, const EqualizerBand &right) {
            return left.type == right.type
                && valuesEqual(left.frequencyHz, right.frequencyHz)
                && valuesEqual(left.gainDb, right.gainDb)
                && valuesEqual(left.q, right.q)
                && left.enabled == right.enabled
                && left.solo == right.solo;
        }

    }

    EqualizerBandModel::EqualizerBandModel(QObject *parent)
        : QAbstractListModel(parent) {
    }

    EqualizerBandModel::~EqualizerBandModel() = default;

    int EqualizerBandModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_bands.size();
    }

    QVariant EqualizerBandModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.column() != 0) {
            return {};
        }
        const auto band = bandAt(index.row());
        if (!band) {
            return {};
        }
        switch (role) {
            case FrequencyHzRole:
                return band->frequencyHz;
            case GainDbRole:
                return band->gainDb;
            case QRole:
                return band->q;
            case TypeRole:
                return static_cast<int>(band->type);
            case EnabledRole:
                return band->enabled;
            case SoloRole:
                return band->solo;
            default:
                return {};
        }
    }

    QHash<int, QByteArray> EqualizerBandModel::roleNames() const {
        return {
            {FrequencyHzRole, "frequencyHz"},
            {GainDbRole, "gainDb"},
            {QRole, "q"},
            {TypeRole, "type"},
            {EnabledRole, "enabled"},
            {SoloRole, "solo"},
        };
    }

    const EqualizerBandList &EqualizerBandModel::bands() const {
        return m_bands;
    }

    const EqualizerBand *EqualizerBandModel::bandAt(int index) const {
        if (index < 0 || index >= m_bands.size()) {
            return nullptr;
        }
        return &m_bands.at(index);
    }

    void EqualizerBandModel::setBands(const EqualizerBandList &bands) {
        beginResetModel();
        m_bands = bands;
        endResetModel();
    }

    void EqualizerBandModel::insertBand(int index, const EqualizerBand &band) {
        Q_ASSERT(index >= 0 && index <= m_bands.size());
        beginInsertRows({}, index, index);
        m_bands.insert(index, band);
        endInsertRows();
    }

    void EqualizerBandModel::removeBand(int index) {
        Q_ASSERT(index >= 0 && index < m_bands.size());
        beginRemoveRows({}, index, index);
        m_bands.removeAt(index);
        endRemoveRows();
    }

    bool EqualizerBandModel::updateBand(int index, const EqualizerBand &band) {
        if (index < 0 || index >= m_bands.size() || bandsEqual(m_bands.at(index), band)) {
            return false;
        }
        auto &current = m_bands[index];
        const bool frequencyChanged = !valuesEqual(current.frequencyHz, band.frequencyHz);
        const bool gainChanged = !valuesEqual(current.gainDb, band.gainDb);
        const bool qChanged = !valuesEqual(current.q, band.q);
        const bool typeChanged = current.type != band.type;
        const bool enabledChanged = current.enabled != band.enabled;
        const bool soloChanged = current.solo != band.solo;
        current = band;

        QList<int> roles;
        if (frequencyChanged) {
            roles.append(FrequencyHzRole);
        }
        if (gainChanged) {
            roles.append(GainDbRole);
        }
        if (qChanged) {
            roles.append(QRole);
        }
        if (typeChanged) {
            roles.append(TypeRole);
        }
        if (enabledChanged) {
            roles.append(EnabledRole);
        }
        if (soloChanged) {
            roles.append(SoloRole);
        }
        const auto modelIndex = this->index(index);
        Q_EMIT dataChanged(modelIndex, modelIndex, roles);
        return true;
    }

}

#include "moc_EqualizerBandModel.cpp"
