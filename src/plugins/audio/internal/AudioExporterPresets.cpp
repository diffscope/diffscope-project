// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioExporterPresets.h"

#include <QJsonDocument>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>

#include <CoreApi/runtimeinterface.h>

namespace Audio::Internal {

    static AudioExporterPresets *m_instance = nullptr;

    static QByteArray serializeConfig(const AudioExporterConfig &config) {
        return QJsonDocument(config.toJsonObject()).toJson(QJsonDocument::Compact);
    }

    static AudioExporterConfig deserializeConfig(const QByteArray &data) {
        const auto document = QJsonDocument::fromJson(data);
        if (!document.isObject()) {
            return {};
        }
        return AudioExporterConfig::fromJsonObject(document.object());
    }

    static QString normalizedPresetName(const QString &name) {
        return name.trimmed();
    }

    AudioExporterPresets::AudioExporterPresets(QObject *parent)
        : QObject(parent), m_model(new QStandardItemModel(this)) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        m_model->setItemRoleNames({
            {NameRole,    "name"   },
            {BuiltinRole, "builtin"},
            {UnsavedRole, "unsaved"},
        });
        initializeBuiltinPresets();
        m_currentPresetId = QStringLiteral("builtin:") + m_builtinPresets.first().id;
        setCurrentConfigInternal(m_builtinPresets.first().config, false);
        rebuildModel();
    }

    AudioExporterPresets::~AudioExporterPresets() {
        save();
        m_instance = nullptr;
    }

    AudioExporterPresets *AudioExporterPresets::instance() {
        return m_instance;
    }

    QAbstractItemModel *AudioExporterPresets::model() const {
        return m_model;
    }

    int AudioExporterPresets::currentIndex() const {
        return m_currentIndex;
    }

    void AudioExporterPresets::setCurrentIndex(int index) {
        const auto builtinCount = m_builtinPresets.size();
        const auto customCount = m_customPresets.size();
        if (index < 0 || index >= m_model->rowCount()) {
            return;
        }
        if (index < builtinCount) {
            loadBuiltinPreset(index);
            return;
        }
        if (index < builtinCount + customCount) {
            loadCustomPreset(m_customPresets.at(index - builtinCount).name);
            return;
        }
        if (!m_dirty) {
            return;
        }
        if (m_currentIndex == index && m_selectionType == SelectionType::Unsaved) {
            return;
        }
        m_selectionType = SelectionType::Unsaved;
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    AudioExporterConfig AudioExporterPresets::currentConfig() const {
        return m_currentConfig;
    }

    void AudioExporterPresets::setCurrentConfig(const AudioExporterConfig &config) {
        setCurrentConfigInternal(config, true);
        m_selectionType = SelectionType::Unsaved;
        m_currentCustomName.clear();
        rebuildModel();
        updateCurrentIndexFromSelection();
    }

    bool AudioExporterPresets::isDirty() const {
        return m_dirty;
    }

    int AudioExporterPresets::builtinPresetCount() const {
        return m_builtinPresets.size();
    }

    QString AudioExporterPresets::builtinPresetName(int builtinIndex) const {
        if (builtinIndex < 0 || builtinIndex >= m_builtinPresets.size()) {
            return {};
        }
        return m_builtinPresets.at(builtinIndex).name;
    }

    AudioExporterConfig AudioExporterPresets::builtinPresetConfig(int builtinIndex) const {
        if (builtinIndex < 0 || builtinIndex >= m_builtinPresets.size()) {
            return {};
        }
        return m_builtinPresets.at(builtinIndex).config;
    }

    bool AudioExporterPresets::loadBuiltinPreset(int builtinIndex) {
        if (builtinIndex < 0 || builtinIndex >= m_builtinPresets.size()) {
            return false;
        }
        m_selectionType = SelectionType::Builtin;
        m_currentBuiltinIndex = builtinIndex;
        m_currentCustomName.clear();
        m_currentPresetId = QStringLiteral("builtin:") + m_builtinPresets.at(builtinIndex).id;
        setCurrentConfigInternal(m_builtinPresets.at(builtinIndex).config, false);
        rebuildModel();
        updateCurrentIndexFromSelection();
        return true;
    }

    QStringList AudioExporterPresets::customPresetNames() const {
        QStringList names;
        names.reserve(m_customPresets.size());
        for (const auto &preset : m_customPresets) {
            names.append(preset.name);
        }
        return names;
    }

    bool AudioExporterPresets::hasCustomPreset(const QString &name) const {
        return customPresetIndex(normalizedPresetName(name)) >= 0;
    }

    AudioExporterConfig AudioExporterPresets::customPresetConfig(const QString &name) const {
        const auto index = customPresetIndex(normalizedPresetName(name));
        if (index < 0) {
            return {};
        }
        return m_customPresets.at(index).config;
    }

    bool AudioExporterPresets::loadCustomPreset(const QString &name) {
        const auto normalizedName = normalizedPresetName(name);
        const auto index = customPresetIndex(normalizedName);
        if (index < 0) {
            return false;
        }
        m_selectionType = SelectionType::Custom;
        m_currentCustomName = normalizedName;
        m_currentPresetId = QStringLiteral("custom:") + normalizedName;
        setCurrentConfigInternal(m_customPresets.at(index).config, false);
        rebuildModel();
        updateCurrentIndexFromSelection();
        return true;
    }

    bool AudioExporterPresets::saveCustomPreset(const QString &name, const AudioExporterConfig &config) {
        const auto normalizedName = normalizedPresetName(name);
        if (normalizedName.isEmpty()) {
            return false;
        }

        const auto index = customPresetIndex(normalizedName);
        if (index >= 0) {
            m_customPresets[index].config = config;
        } else {
            m_customPresets.append({normalizedName, normalizedName, config});
        }

        m_selectionType = SelectionType::Custom;
        m_currentCustomName = normalizedName;
        m_currentPresetId = QStringLiteral("custom:") + normalizedName;
        setCurrentConfigInternal(config, false);
        rebuildModel();
        updateCurrentIndexFromSelection();
        emit customPresetsChanged();
        return true;
    }

    bool AudioExporterPresets::saveCurrentAsCustomPreset(const QString &name) {
        return saveCustomPreset(name, m_currentConfig);
    }

    bool AudioExporterPresets::removeCustomPreset(const QString &name) {
        const auto normalizedName = normalizedPresetName(name);
        const auto index = customPresetIndex(normalizedName);
        if (index < 0) {
            return false;
        }

        const auto removingCurrent = m_selectionType == SelectionType::Custom && m_currentCustomName == normalizedName;
        const auto removingSavedSelection = m_currentPresetId == QStringLiteral("custom:") + normalizedName;
        m_customPresets.removeAt(index);
        if (removingCurrent) {
            m_selectionType = SelectionType::Builtin;
            m_currentBuiltinIndex = 0;
            m_currentCustomName.clear();
            m_currentPresetId = QStringLiteral("builtin:") + m_builtinPresets.first().id;
            setCurrentConfigInternal(m_builtinPresets.first().config, false);
        } else if (removingSavedSelection) {
            m_currentPresetId = QStringLiteral("builtin:") + m_builtinPresets.first().id;
        }
        rebuildModel();
        updateCurrentIndexFromSelection();
        emit customPresetsChanged();
        return true;
    }

    void AudioExporterPresets::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.audio.audioExporter"));
        const auto presets = settings->value(QStringLiteral("customPresets")).toList();
        const auto currentPresetId = settings->value(QStringLiteral("currentPresetId")).toString();
        settings->endGroup();

        auto state = Core::RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.audio.audioExporter"));
        const auto dirty = state->value(QStringLiteral("dirty"), false).toBool();
        const auto unsavedConfig = QJsonDocument::fromJson(state->value(QStringLiteral("unsavedConfig")).toByteArray()).object();
        state->endGroup();

        m_customPresets.clear();
        for (const auto &value : presets) {
            const auto data = value.toMap();
            const auto name = normalizedPresetName(data.value(QStringLiteral("name")).toString());
            if (name.isEmpty() || hasCustomPreset(name)) {
                continue;
            }
            m_customPresets.append({
                name,
                name,
                deserializeConfig(data.value(QStringLiteral("config")).toByteArray()),
            });
        }

        m_selectionType = SelectionType::Builtin;
        m_currentBuiltinIndex = 0;
        m_currentCustomName.clear();
        m_currentPresetId = QStringLiteral("builtin:") + m_builtinPresets.first().id;

        if (currentPresetId.startsWith(QStringLiteral("builtin:"))) {
            const auto builtinId = currentPresetId.sliced(8);
            for (int i = 0; i < m_builtinPresets.size(); ++i) {
                if (m_builtinPresets.at(i).id == builtinId) {
                    m_currentBuiltinIndex = i;
                    m_currentPresetId = currentPresetId;
                    break;
                }
            }
        } else if (currentPresetId.startsWith(QStringLiteral("custom:"))) {
            const auto name = currentPresetId.sliced(7);
            if (hasCustomPreset(name)) {
                m_selectionType = SelectionType::Custom;
                m_currentCustomName = name;
                m_currentPresetId = currentPresetId;
            }
        }

        if (m_selectionType == SelectionType::Custom) {
            m_currentConfig = customPresetConfig(m_currentCustomName);
        } else {
            m_selectionType = SelectionType::Builtin;
            m_currentCustomName.clear();
            m_currentConfig = m_builtinPresets.at(m_currentBuiltinIndex).config;
        }

        if (dirty && !unsavedConfig.isEmpty()) {
            m_selectionType = SelectionType::Unsaved;
            m_dirty = true;
            m_currentConfig = AudioExporterConfig::fromJsonObject(unsavedConfig);
        } else {
            m_dirty = false;
        }

        rebuildModel();
        updateCurrentIndexFromSelection();
        emit currentConfigChanged();
        emit dirtyChanged();
        emit customPresetsChanged();
    }

    void AudioExporterPresets::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.audio.audioExporter"));

        QVariantList presets;
        presets.reserve(m_customPresets.size());
        for (const auto &preset : m_customPresets) {
            presets.append(QVariantMap{
                {QStringLiteral("name"),   preset.name                     },
                {QStringLiteral("config"), serializeConfig(preset.config)  },
            });
        }

        settings->setValue(QStringLiteral("customPresets"), presets);
        settings->setValue(QStringLiteral("currentPresetId"), m_currentPresetId);
        settings->endGroup();

        auto state = Core::RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.audio.audioExporter"));
        state->setValue(QStringLiteral("dirty"), m_dirty);
        state->setValue(QStringLiteral("unsavedConfig"), serializeConfig(m_currentConfig));
        state->endGroup();
    }

    void AudioExporterPresets::initializeBuiltinPresets() {
        const auto makePreset = [](const QString &fileName, AudioExporterConfig::FileType fileType, AudioExporterConfig::MixingOption mixingOption) {
            AudioExporterConfig config;
            config.setFileName(fileName);
            config.setFileDirectory({});
            config.setFileType(fileType);
            config.setFormatMono(false);
            config.setFormatOption(0);
            config.setFormatQuality(100);
            config.setFormatSampleRate(48000);
            config.setMixingOption(mixingOption);
            config.setMuteSoloEnabled(true);
            config.setEffectsEnabled(true);
            config.setSourceOption(AudioExporterConfig::SO_All);
            config.setTimeRange(AudioExporterConfig::TR_All);
            return config;
        };

        const auto wavMix = makePreset(QStringLiteral("${projectName}.wav"), AudioExporterConfig::FT_Wav, AudioExporterConfig::MO_Mixed);
        const auto wavSep = makePreset(QStringLiteral("${projectName}_${trackIndex}_${trackName}.wav"), AudioExporterConfig::FT_Wav, AudioExporterConfig::MO_SeparatedThruMaster);
        const auto flacMix = makePreset(QStringLiteral("${projectName}.flac"), AudioExporterConfig::FT_Flac, AudioExporterConfig::MO_Mixed);
        const auto flacSep = makePreset(QStringLiteral("${projectName}_${trackIndex}_${trackName}.flac"), AudioExporterConfig::FT_Flac, AudioExporterConfig::MO_SeparatedThruMaster);
        const auto oggMix = makePreset(QStringLiteral("${projectName}.ogg"), AudioExporterConfig::FT_OggContainer, AudioExporterConfig::MO_Mixed);
        const auto oggSep = makePreset(QStringLiteral("${projectName}_${trackIndex}_${trackName}.ogg"), AudioExporterConfig::FT_OggContainer, AudioExporterConfig::MO_SeparatedThruMaster);

        m_builtinPresets = {
            {QStringLiteral("wavMixed"),       tr("WAV (Mixed)"),               wavMix },
            {QStringLiteral("wavSeparated"),   tr("WAV (Separated)"),           wavSep },
            {QStringLiteral("flacMixed"),      tr("FLAC (Mixed)"),              flacMix},
            {QStringLiteral("flacSeparated"),  tr("FLAC (Separated)"),          flacSep},
            {QStringLiteral("oggMixed"),       tr("Ogg Container (Mixed)"),     oggMix },
            {QStringLiteral("oggSeparated"),   tr("Ogg Container (Separated)"), oggSep },
        };
    }

    void AudioExporterPresets::rebuildModel() {
        m_model->clear();
        m_model->setItemRoleNames({
            {NameRole,    "name"   },
            {BuiltinRole, "builtin"},
            {UnsavedRole, "unsaved"},
        });

        const auto appendPreset = [this](const QString &name, bool builtin, bool unsaved) {
            auto item = new QStandardItem(name);
            item->setData(name, NameRole);
            item->setData(builtin, BuiltinRole);
            item->setData(unsaved, UnsavedRole);
            m_model->appendRow(item);
        };

        for (const auto &preset : m_builtinPresets) {
            appendPreset(preset.name, true, false);
        }
        for (const auto &preset : m_customPresets) {
            appendPreset(preset.name, false, false);
        }
        if (m_dirty) {
            appendPreset(tr("Unsaved Preset"), false, true);
        }
    }

    void AudioExporterPresets::updateCurrentIndexFromSelection() {
        int index = 0;
        switch (m_selectionType) {
            case SelectionType::Builtin:
                index = qBound(0, m_currentBuiltinIndex, m_builtinPresets.size() - 1);
                break;
            case SelectionType::Custom: {
                const auto customIndex = customPresetIndex(m_currentCustomName);
                index = customIndex >= 0 ? m_builtinPresets.size() + customIndex : 0;
                break;
            }
            case SelectionType::Unsaved:
                index = m_dirty ? m_model->rowCount() - 1 : 0;
                break;
        }
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    void AudioExporterPresets::setCurrentConfigInternal(const AudioExporterConfig &config, bool dirty) {
        const auto configChanged = serializeConfig(m_currentConfig) != serializeConfig(config);
        const auto dirtyChanged = m_dirty != dirty;
        m_currentConfig = config;
        m_dirty = dirty;
        if (configChanged) {
            emit this->currentConfigChanged();
        }
        if (dirtyChanged) {
            emit this->dirtyChanged();
        }
    }

    int AudioExporterPresets::customPresetIndex(const QString &name) const {
        for (int i = 0; i < m_customPresets.size(); ++i) {
            if (m_customPresets.at(i).name == name) {
                return i;
            }
        }
        return -1;
    }

}

#include "moc_AudioExporterPresets.cpp"
