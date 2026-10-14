// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ACTIONCONFIGURATION_H
#define DIFFSCOPE_COREPLUGIN_ACTIONCONFIGURATION_H

#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

namespace QAK {
    class ActionLayouts;
}

namespace Core::Internal {

    using KeymapOverrides = QMap<QString, QKeySequence>;

    struct KeymapPreset {
        QString id;
        QString name;
        KeymapOverrides overrides;
    };

    class ActionConfiguration : public QObject {
        Q_OBJECT
    public:
        explicit ActionConfiguration(QObject *parent = nullptr);
        static ActionConfiguration *get();

        void load();
        bool saveLayouts(const QAK::ActionLayouts &layouts);
        bool saveKeymaps(const QList<KeymapPreset> &presets, const QString &currentId);
        QList<KeymapPreset> keymapPresets() const;
        QString currentKeymapId() const;

    private:
        QList<KeymapPreset> m_keymapPresets;
        QString m_currentKeymapId;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_ACTIONCONFIGURATION_H
