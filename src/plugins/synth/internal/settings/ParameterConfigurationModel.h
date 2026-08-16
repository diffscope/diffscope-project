// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_PARAMETERCONFIGURATIONMODEL_H
#define DIFFSCOPE_SYNTH_PARAMETERCONFIGURATIONMODEL_H

#include <QAbstractListModel>

#include <synth/ParameterConfiguration.h>

namespace Synth::Internal {

    class ParameterConfigurationModel final : public QAbstractListModel {
        Q_OBJECT
    public:
        enum Role {
            IdRole = Qt::UserRole + 1,
            ArchitectureIdRole,
            DisplayNameRole,
            MinimumValueRole,
            MaximumValueRole,
            ShowDefaultValueRole,
            DefaultValueRole,
            FillModeRole,
            ValueTypeRole,
            ShowDivisionRole,
            DivisionValueRole,
            NormalizationExpressionRole,
            DenormalizationExpressionRole,
            DisplayValueExpressionRole,
            DisplayValueInverseExpressionRole,
            DisplayTextTemplateRole,
            BuiltinRole,
        };
        Q_ENUM(Role)

        explicit ParameterConfigurationModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QHash<int, QByteArray> roleNames() const override;

        void setConfigurations(const QList<ParameterConfiguration> &allConfigurations,
                               const QList<ParameterConfiguration> &userConfigurations);
        QList<ParameterConfiguration> allConfigurations() const;
        QList<ParameterConfiguration> userConfigurations() const;
        bool isBuiltinId(const QString &id) const;

        bool mergeImported(const QList<ParameterConfiguration> &configurations,
                           QStringList *ignoredIds = nullptr);

        Q_INVOKABLE int addParameter();
        Q_INVOKABLE bool removeParameter(int row);

    Q_SIGNALS:
        void edited();

    private:
        struct Entry {
            ParameterConfiguration configuration;
            bool builtin{};
        };

        static bool entryLess(const Entry &left, const Entry &right);
        void moveEntryToSortedPosition(int row);
        void sortEntries();
        QList<Entry> m_entries;
    };

}

#endif // DIFFSCOPE_SYNTH_PARAMETERCONFIGURATIONMODEL_H
