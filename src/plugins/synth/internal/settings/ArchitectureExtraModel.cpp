// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ArchitectureExtraModel.h"

#include <algorithm>

#include <QJsonParseError>
#include <QSet>

namespace Synth::Internal {

    ArchitectureExtraModel::ArchitectureExtraModel(QObject *parent)
        : QAbstractListModel(parent) {
    }

    int ArchitectureExtraModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_entries.size();
    }

    QVariant ArchitectureExtraModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return {};
        const auto &entry = m_entries.at(index.row());
        if (role == ArchitectureIdRole)
            return entry.architectureId;
        if (role == JsonRole)
            return entry.json;
        return {};
    }

    bool ArchitectureExtraModel::setData(const QModelIndex &index, const QVariant &value, int role) {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return false;
        auto &entry = m_entries[index.row()];
        const auto text = value.toString();
        if (role == ArchitectureIdRole) {
            if (entry.architectureId == text)
                return true;
            entry.architectureId = text;
        } else if (role == JsonRole) {
            if (entry.json == text)
                return true;
            entry.json = text;
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
            {JsonRole, "json"},
        };
    }

    int ArchitectureExtraModel::addEntry() {
        const int row = m_entries.size();
        beginInsertRows({}, row, row);
        m_entries.append({{}, QStringLiteral("{}")});
        endInsertRows();
        Q_EMIT edited();
        return row;
    }

    bool ArchitectureExtraModel::removeEntry(int row) {
        if (row < 0 || row >= m_entries.size())
            return false;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        Q_EMIT edited();
        return true;
    }

    void ArchitectureExtraModel::setEntries(const QJsonObject &object) {
        beginResetModel();
        m_entries.clear();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            const QJsonValue value = it.value();
            const QByteArray encoded = value.toJson();
            m_entries.append({it.key(), QString::fromUtf8(encoded).trimmed()});
        }
        std::sort(m_entries.begin(), m_entries.end(), [](const Entry &left, const Entry &right) {
            return left.architectureId < right.architectureId;
        });
        endResetModel();
    }

    bool ArchitectureExtraModel::entries(QJsonObject *object, QString *errorMessage) const {
        QJsonObject result;
        QSet<QString> ids;
        for (int index = 0; index < m_entries.size(); ++index) {
            const auto id = m_entries.at(index).architectureId.trimmed();
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
            QJsonParseError error;
            const auto value = QJsonValue::fromJson(m_entries.at(index).json.trimmed().toUtf8(), &error);
            if (error.error != QJsonParseError::NoError || value.isUndefined()) {
                if (errorMessage)
                    *errorMessage = tr("Invalid JSON for architecture '%1': %2").arg(id, error.errorString());
                return false;
            }
            ids.insert(id);
            result.insert(id, value);
        }
        if (object)
            *object = result;
        return true;
    }

}
