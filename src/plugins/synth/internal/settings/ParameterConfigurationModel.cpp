// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterConfigurationModel.h"

#include <algorithm>
#include <utility>

#include <QMap>
#include <QSet>

namespace Synth::Internal {

    ParameterConfigurationModel::ParameterConfigurationModel(QObject *parent)
        : QAbstractListModel(parent) {
    }

    int ParameterConfigurationModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_entries.size();
    }

    QVariant ParameterConfigurationModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return {};
        const auto &entry = m_entries.at(index.row());
        const auto &configuration = entry.configuration;
        switch (role) {
            case IdRole:
                return configuration.id();
            case ArchitectureIdRole:
                return configuration.architectureId();
            case DisplayNameRole:
                return configuration.displayName();
            case ShowDefaultValueRole:
                return configuration.showDefaultValue();
            case DefaultValueRole:
                return configuration.defaultValue();
            case FillModeRole:
                return static_cast<int>(configuration.fillMode());
            case ValueTypeRole:
                return static_cast<int>(configuration.valueType());
            case ShowDivisionRole:
                return configuration.showDivision();
            case DivisionValueRole:
                return configuration.divisionValue();
            case DisplayValueExpressionRole:
                return configuration.displayValueExpression();
            case DisplayValueInverseExpressionRole:
                return configuration.displayValueInverseExpression();
            case DisplayTextTemplateRole:
                return configuration.displayTextTemplate();
            case BuiltinRole:
                return entry.builtin;
            default:
                return {};
        }
    }

    bool ParameterConfigurationModel::setData(const QModelIndex &index, const QVariant &value, int role) {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
            return false;
        auto &entry = m_entries[index.row()];
        if (entry.builtin)
            return false;

        auto &configuration = entry.configuration;
        switch (role) {
            case IdRole:
                configuration.setId(value.toString());
                break;
            case ArchitectureIdRole:
                configuration.setArchitectureId(value.toString());
                break;
            case DisplayNameRole:
                configuration.setDisplayName(value.toString());
                break;
            case ShowDefaultValueRole:
                configuration.setShowDefaultValue(value.toBool());
                break;
            case DefaultValueRole:
                configuration.setDefaultValue(value.toDouble());
                break;
            case FillModeRole:
                configuration.setFillMode(static_cast<ParameterConfiguration::FillMode>(value.toInt()));
                break;
            case ValueTypeRole:
                configuration.setValueType(static_cast<ParameterConfiguration::ValueType>(value.toInt()));
                break;
            case ShowDivisionRole:
                configuration.setShowDivision(value.toBool());
                break;
            case DivisionValueRole:
                configuration.setDivisionValue(value.toDouble());
                break;
            case DisplayValueExpressionRole:
                configuration.setDisplayValueExpression(value.toString());
                break;
            case DisplayValueInverseExpressionRole:
                configuration.setDisplayValueInverseExpression(value.toString());
                break;
            case DisplayTextTemplateRole:
                configuration.setDisplayTextTemplate(value.toString());
                break;
            default:
                return false;
        }
        Q_EMIT dataChanged(index, index, {role});
        if (role == IdRole || role == ArchitectureIdRole)
            moveEntryToSortedPosition(index.row());
        Q_EMIT edited();
        return true;
    }

    Qt::ItemFlags ParameterConfigurationModel::flags(const QModelIndex &index) const {
        auto result = QAbstractListModel::flags(index);
        if (index.isValid() && index.row() >= 0 && index.row() < m_entries.size() && !m_entries.at(index.row()).builtin)
            result |= Qt::ItemIsEditable;
        return result;
    }

    QHash<int, QByteArray> ParameterConfigurationModel::roleNames() const {
        return {
            {IdRole, "parameterId"},
            {ArchitectureIdRole, "architectureId"},
            {DisplayNameRole, "displayName"},
            {ShowDefaultValueRole, "showDefaultValue"},
            {DefaultValueRole, "defaultValue"},
            {FillModeRole, "fillMode"},
            {ValueTypeRole, "valueType"},
            {ShowDivisionRole, "showDivision"},
            {DivisionValueRole, "divisionValue"},
            {DisplayValueExpressionRole, "displayValueExpression"},
            {DisplayValueInverseExpressionRole, "displayValueInverseExpression"},
            {DisplayTextTemplateRole, "displayTextTemplate"},
            {BuiltinRole, "builtin"},
        };
    }

    void ParameterConfigurationModel::setConfigurations(
        const QList<ParameterConfiguration> &allConfigurations,
        const QList<ParameterConfiguration> &userConfigurations) {
        QSet<QString> userIds;
        for (const auto &configuration : userConfigurations)
            userIds.insert(configuration.id());

        beginResetModel();
        m_entries.clear();
        m_entries.reserve(allConfigurations.size());
        for (const auto &configuration : allConfigurations)
            m_entries.append({configuration, !userIds.contains(configuration.id())});
        sortEntries();
        endResetModel();
    }

    QList<ParameterConfiguration> ParameterConfigurationModel::allConfigurations() const {
        QList<ParameterConfiguration> result;
        result.reserve(m_entries.size());
        for (const auto &entry : m_entries)
            result.append(entry.configuration);
        return result;
    }

    QList<ParameterConfiguration> ParameterConfigurationModel::userConfigurations() const {
        QList<ParameterConfiguration> result;
        for (const auto &entry : m_entries) {
            if (!entry.builtin)
                result.append(entry.configuration);
        }
        return result;
    }

    bool ParameterConfigurationModel::isBuiltinId(const QString &id) const {
        return std::ranges::any_of(m_entries, [&id](const Entry &entry) {
            return entry.builtin && entry.configuration.id() == id;
        });
    }

    bool ParameterConfigurationModel::mergeImported(
        const QList<ParameterConfiguration> &configurations,
        QStringList *ignoredIds) {
        QSet<QString> builtinIds;
        QMap<QString, Entry> merged;
        for (const auto &entry : std::as_const(m_entries)) {
            if (entry.builtin)
                builtinIds.insert(entry.configuration.id());
            merged.insert(entry.configuration.id(), entry);
        }

        for (const auto &configuration : configurations) {
            if (configuration.id() == QStringLiteral("pitch") || builtinIds.contains(configuration.id())) {
                if (ignoredIds)
                    ignoredIds->append(configuration.id());
                continue;
            }
            merged.insert(configuration.id(), {configuration, false});
        }

        beginResetModel();
        m_entries = merged.values();
        sortEntries();
        endResetModel();
        Q_EMIT edited();
        return true;
    }

    int ParameterConfigurationModel::addParameter() {
        QSet<QString> ids;
        for (const auto &entry : std::as_const(m_entries))
            ids.insert(entry.configuration.id());

        QString id = QStringLiteral("parameter");
        for (int suffix = 2; ids.contains(id); ++suffix)
            id = QStringLiteral("parameter%1").arg(suffix);

        ParameterConfiguration configuration;
        configuration.setId(id);
        configuration.setArchitectureId(QStringLiteral("diffsinger"));
        configuration.setDisplayName(tr("New Parameter"));
        configuration.setShowDefaultValue(false);
        configuration.setDefaultValue(0.0);
        configuration.setFillMode(ParameterConfiguration::NoFill);
        configuration.setValueType(ParameterConfiguration::Absolute);
        configuration.setShowDivision(true);
        configuration.setDivisionValue(0.2);
        configuration.setDisplayValueExpression(QStringLiteral("x"));
        configuration.setDisplayValueInverseExpression(QStringLiteral("x"));
        configuration.setDisplayTextTemplate(QStringLiteral("%d"));

        const Entry entry{configuration, false};
        const auto position = std::ranges::lower_bound(m_entries, entry, entryLess);
        const int row = static_cast<int>(std::distance(m_entries.begin(), position));
        beginInsertRows({}, row, row);
        m_entries.insert(row, entry);
        endInsertRows();
        Q_EMIT edited();
        return row;
    }

    bool ParameterConfigurationModel::removeParameter(int row) {
        if (row < 0 || row >= m_entries.size() || m_entries.at(row).builtin)
            return false;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        Q_EMIT edited();
        return true;
    }

    bool ParameterConfigurationModel::entryLess(const Entry &left, const Entry &right) {
        const int architectureComparison = QString::compare(
            left.configuration.architectureId(), right.configuration.architectureId(), Qt::CaseSensitive);
        if (architectureComparison != 0)
            return architectureComparison < 0;
        return left.configuration.id() < right.configuration.id();
    }

    void ParameterConfigurationModel::moveEntryToSortedPosition(int row) {
        if (row < 0 || row >= m_entries.size())
            return;

        const auto entry = m_entries.at(row);
        auto withoutEntry = m_entries;
        withoutEntry.removeAt(row);
        const auto position = std::ranges::lower_bound(withoutEntry, entry, entryLess);
        const int targetRow = static_cast<int>(std::distance(withoutEntry.begin(), position));
        if (targetRow == row)
            return;

        const int destinationChild = targetRow > row ? targetRow + 1 : targetRow;
        beginMoveRows({}, row, row, {}, destinationChild);
        m_entries.move(row, targetRow);
        endMoveRows();
    }

    void ParameterConfigurationModel::sortEntries() {
        std::ranges::sort(m_entries, entryLess);
    }

}
