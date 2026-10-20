// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ArchitectureExtraModel.h"

#include <algorithm>

#include <QJSValue>
#include <QMap>
#include <QSet>

#include <synth/SynthInterface.h>

namespace Synth::Internal {

    ArchitectureExtraModel::ArchitectureExtraModel(QObject *parent)
        : QAbstractListModel(parent) {
        connect(SynthInterface::instance(), &SynthInterface::controlPanelsChanged, this, &ArchitectureExtraModel::syncBuiltInEntries);
    }

    int ArchitectureExtraModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_entries.size();
    }

    QVariant ArchitectureExtraModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return {};
        const auto &entry = m_entries.at(index.row());
        switch (role) {
            case ArchitectureIdRole:
                return entry.architectureId;
            case ExtraRole:
                return QVariant::fromValue(entry.extra);
            case ProviderUrlRole:
                return entry.providerUrl;
            case BuiltInRole:
                return entry.builtIn;
            default:
                return {};
        }
    }

    bool ArchitectureExtraModel::setData(const QModelIndex &index, const QVariant &value, int role) {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return false;
        auto &entry = m_entries[index.row()];
        if (role == ArchitectureIdRole) {
            if (entry.builtIn)
                return false;
            const auto id = value.toString();
            if (entry.architectureId == id)
                return true;
            entry.architectureId = id;
        } else if (role == ExtraRole) {
            const auto variant = value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
            const auto extra = variant.metaType() == QMetaType::fromType<QJsonValue>() ? variant.value<QJsonValue>() : QJsonValue::fromVariant(variant);
            if (extra.isUndefined())
                return false;
            if (entry.extra == extra)
                return true;
            entry.extra = extra;
            entry.extraConfigured = true;
        } else if (role == ProviderUrlRole) {
            const auto url = value.toUrl();
            if (!url.isEmpty() && (!url.isValid() || !url.isLocalFile()))
                return false;
            if (entry.providerUrl == url)
                return true;
            entry.providerUrl = url;
        } else {
            return false;
        }
        Q_EMIT dataChanged(index, index, {role});
        Q_EMIT edited();
        return true;
    }

    Qt::ItemFlags ArchitectureExtraModel::flags(const QModelIndex &index) const {
        return QAbstractListModel::flags(index) | (index.isValid() ? Qt::ItemIsEditable : Qt::NoItemFlags);
    }

    QHash<int, QByteArray> ArchitectureExtraModel::roleNames() const {
        return {
            {ArchitectureIdRole, "architectureId"},
            {ExtraRole, "extra"},
            {ProviderUrlRole, "providerUrl"},
            {BuiltInRole, "builtIn"},
        };
    }

    int ArchitectureExtraModel::addEntry() {
        const int row = m_entries.size();
        beginInsertRows({}, row, row);
        m_entries.append({{}, QJsonObject{}, {}, true});
        endInsertRows();
        Q_EMIT edited();
        return row;
    }

    bool ArchitectureExtraModel::removeEntry(int row) {
        if (row < 0 || row >= m_entries.size() || m_entries.at(row).builtIn)
            return false;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        Q_EMIT edited();
        return true;
    }

    void ArchitectureExtraModel::setEntries(const QJsonObject &extras, const QJsonObject &providers) {
        QMap<QString, QJsonValue> defaults;
        const auto interface = SynthInterface::instance();
        const auto builtInIds = interface->builtInArchitectureIds();
        QSet<QString> listedIds(builtInIds.cbegin(), builtInIds.cend());
        for (auto it = providers.begin(); it != providers.end(); ++it)
            listedIds.insert(it.key());
        for (auto it = extras.begin(); it != extras.end(); ++it)
            listedIds.insert(it.key());
        for (const auto &service : interface->serviceInstances()) {
            if (!service.isEnabled())
                continue;
            for (const auto &architecture : interface->serviceInstanceDetails(service.id()).metadata().architectures()) {
                if (listedIds.contains(architecture.id()) && !defaults.contains(architecture.id()))
                    defaults.insert(architecture.id(), architecture.defaultArchExtra());
            }
        }
        for (const auto &id : builtInIds) {
            if (!defaults.contains(id))
                defaults.insert(id, QJsonObject{});
        }
        for (auto it = providers.begin(); it != providers.end(); ++it) {
            if (!defaults.contains(it.key()))
                defaults.insert(it.key(), QJsonObject{});
        }
        for (auto it = extras.begin(); it != extras.end(); ++it)
            defaults.insert(it.key(), it.value());

        beginResetModel();
        m_entries.clear();
        for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
            m_entries.append({it.key(), it.value(), QUrl(providers.value(it.key()).toString()), extras.contains(it.key()), interface->isBuiltInArchitecture(it.key())});
        endResetModel();
    }

    void ArchitectureExtraModel::syncBuiltInEntries() {
        for (const auto &id : SynthInterface::instance()->builtInArchitectureIds()) {
            const auto existing = std::ranges::find(m_entries, id, &Entry::architectureId);
            if (existing != m_entries.end()) {
                existing->builtIn = true;
                const auto modelIndex = index(static_cast<int>(existing - m_entries.begin()));
                Q_EMIT dataChanged(modelIndex, modelIndex, {BuiltInRole});
                continue;
            }
            const int row = m_entries.size();
            beginInsertRows({}, row, row);
            m_entries.append({id, QJsonObject{}, {}, false, true});
            endInsertRows();
        }
    }

    bool ArchitectureExtraModel::entries(QJsonObject *extras, QJsonObject *providers, QString *errorMessage) const {
        QJsonObject resultExtras;
        QJsonObject resultProviders;
        QSet<QString> ids;
        for (int index = 0; index < m_entries.size(); ++index) {
            const auto &entry = m_entries.at(index);
            const auto id = entry.architectureId.trimmed();
            if (id.isEmpty()) {
                if (errorMessage)
                    *errorMessage = tr("Architecture entry %L1 has no ID").arg(index + 1);
                return false;
            }
            if (ids.contains(id)) {
                if (errorMessage)
                    *errorMessage = tr("Architecture ID '%1' is configured more than once").arg(id);
                return false;
            }
            ids.insert(id);
            if (entry.extraConfigured)
                resultExtras.insert(id, entry.extra);
            if (!entry.providerUrl.isEmpty())
                resultProviders.insert(id, entry.providerUrl.toString());
        }
        if (extras)
            *extras = resultExtras;
        if (providers)
            *providers = resultProviders;
        return true;
    }

}
