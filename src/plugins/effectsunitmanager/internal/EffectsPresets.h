// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPRESETS_H
#define DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPRESETS_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QStringList>

#include <stdcorelib/support/json.h>

namespace EffectsUnitManager::Internal {

    class EffectsPresets : public QObject {
        Q_OBJECT
    public:
        static EffectsPresets *instance();

        explicit EffectsPresets(QObject *parent = nullptr);
        ~EffectsPresets() override;

        QStringList presetNames() const;
        bool hasPreset(const QString &name) const;
        bool savePreset(const QString &name, const stdc::JsonArray &audioDSPs);
        bool removePreset(const QString &name);
        stdc::JsonArray presetAudioDSPs(const QString &name) const;

        void load();
        void save() const;

    Q_SIGNALS:
        void presetsChanged();

    private:
        struct Preset {
            QString name;
            QByteArray data;
        };

        QByteArray serialize(const stdc::JsonArray &audioDSPs) const;
        stdc::JsonArray deserialize(const QByteArray &data) const;

        static EffectsPresets *m_instance;
        QList<Preset> m_presets;
    };

}

#endif // DIFFSCOPE_EFFECTS_UNIT_MANAGER_EFFECTSPRESETS_H
