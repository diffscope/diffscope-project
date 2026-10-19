// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_ARCHITECTUREEXTRAMODEL_H
#define DIFFSCOPE_SYNTH_ARCHITECTUREEXTRAMODEL_H

#include <QAbstractListModel>
#include <QJsonObject>
#include <QUrl>

namespace Synth::Internal {

    class ArchitectureExtraModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum Role {
            ArchitectureIdRole = Qt::UserRole + 1,
            ExtraRole,
            ProviderUrlRole,
            BuiltInRole,
        };
        Q_ENUM(Role)

        explicit ArchitectureExtraModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QHash<int, QByteArray> roleNames() const override;

        Q_INVOKABLE int addEntry();
        Q_INVOKABLE bool removeEntry(int row);
        void setEntries(const QJsonObject &extras, const QJsonObject &providers);
        bool entries(QJsonObject *extras, QJsonObject *providers, QString *errorMessage = nullptr) const;

    Q_SIGNALS:
        void edited();

    private:
        void syncBuiltInEntries();

        struct Entry {
            QString architectureId;
            QJsonValue extra;
            QUrl providerUrl;
            bool extraConfigured{};
            bool builtIn{};
        };
        QList<Entry> m_entries;
    };

}

#endif // DIFFSCOPE_SYNTH_ARCHITECTUREEXTRAMODEL_H
