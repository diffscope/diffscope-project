// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "MixedSingerPresets.h"
#include "MixedSingerPresets_p.h"

#include <algorithm>

#include <QSettings>
#include <QUuid>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/SourcesPickerModel.h>

namespace Core {

    void MixedSingerPresetsPrivate::load() {
        Q_Q(MixedSingerPresets);
        auto *settings = RuntimeInterface::settings();
        settings->beginGroup(MixedSingerPresets::staticMetaObject.className());
        const auto storedPresets = settings->value(QStringLiteral("presets")).toList();
        settings->endGroup();
        for (const auto &value : storedPresets) {
            const auto entry = value.toMap();
            const auto id = entry.value(QStringLiteral("id")).toString();
            const auto name = entry.value(QStringLiteral("name")).toString().trimmed();
            if (id.isEmpty() || name.isEmpty() || std::ranges::any_of(presets, [&id](const auto &preset) { return preset.id == id; }))
                continue;
            auto *sources = new SourcesPickerModel(q);
            if (!sources->deserialize(entry.value(QStringLiteral("data")).toByteArray()) || sources->count() != 1
                || sources->singerKind(sources->modelIndex(0)) != SourcesPickerModel::MixedSinger) {
                delete sources;
                continue;
            }
            sources->setMixedName(sources->modelIndex(0), name);
            presets.append({id, name, sources});
        }
    }

    void MixedSingerPresetsPrivate::save() const {
        QVariantList entries;
        entries.reserve(presets.size());
        for (const auto &preset : presets) {
            entries.append(QVariantMap{
                {QStringLiteral("id"), preset.id},
                {QStringLiteral("name"), preset.name},
                {QStringLiteral("data"), preset.sources->serialize()},
            });
        }
        auto *settings = RuntimeInterface::settings();
        settings->beginGroup(MixedSingerPresets::staticMetaObject.className());
        settings->setValue(QStringLiteral("presets"), entries);
        settings->endGroup();
    }

    MixedSingerPresets::MixedSingerPresets(QObject *parent) : QObject(parent), d_ptr(new MixedSingerPresetsPrivate) {
        Q_D(MixedSingerPresets);
        d->q_ptr = this;
        d->load();
    }

    MixedSingerPresets::~MixedSingerPresets() = default;

    QVariantList MixedSingerPresets::presets() const {
        Q_D(const MixedSingerPresets);
        QVariantList result;
        result.reserve(d->presets.size());
        for (const auto &preset : d->presets) {
            result.append(QVariantMap{
                {QStringLiteral("id"), preset.id},
                {QStringLiteral("name"), preset.name},
                {QStringLiteral("sourcesModel"), QVariant::fromValue(preset.sources)},
            });
        }
        return result;
    }

    bool MixedSingerPresets::savePreset(const QString &name, SourcesPickerModel *sources, const QModelIndex &index) {
        Q_D(MixedSingerPresets);
        const auto trimmedName = name.trimmed();
        if (trimmedName.isEmpty() || !sources || sources->singerKind(index) != SourcesPickerModel::MixedSinger)
            return false;
        auto *snapshot = new SourcesPickerModel(this);
        snapshot->setArchitectureId(sources->architectureId());
        snapshot->setSingers({sources->singer(index)});
        if (!snapshot->valid()) {
            delete snapshot;
            return false;
        }
        snapshot->setMixedName(snapshot->modelIndex(0), trimmedName);
        if (snapshot->serialize().isEmpty()) {
            delete snapshot;
            return false;
        }
        d->presets.append({QUuid::createUuid().toString(QUuid::WithoutBraces), trimmedName, snapshot});
        d->save();
        Q_EMIT presetsChanged();
        return true;
    }

    bool MixedSingerPresets::renamePreset(const QString &id, const QString &name) {
        Q_D(MixedSingerPresets);
        const auto trimmedName = name.trimmed();
        const auto it = std::ranges::find(d->presets, id, &MixedSingerPresetsPrivate::Preset::id);
        if (it == d->presets.end() || trimmedName.isEmpty())
            return false;
        if (it->name == trimmedName)
            return true;
        it->name = trimmedName;
        it->sources->setMixedName(it->sources->modelIndex(0), trimmedName);
        d->save();
        Q_EMIT presetsChanged();
        return true;
    }

    bool MixedSingerPresets::removePreset(const QString &id) {
        Q_D(MixedSingerPresets);
        const auto it = std::ranges::find(d->presets, id, &MixedSingerPresetsPrivate::Preset::id);
        if (it == d->presets.end())
            return false;
        auto *sources = it->sources;
        d->presets.erase(it);
        d->save();
        Q_EMIT presetsChanged();
        sources->deleteLater();
        return true;
    }

}

#include "moc_MixedSingerPresets.cpp"
