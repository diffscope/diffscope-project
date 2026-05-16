#ifndef DIFFSCOPE_AUDIO_AUDIOEXPORTERPRESETS_H
#define DIFFSCOPE_AUDIO_AUDIOEXPORTERPRESETS_H

#include <QObject>
#include <QStringList>
#include <QtCore/Qt>
#include <qqmlintegration.h>

#include <audio/AudioExporterConfig.h>

class QAbstractItemModel;
class QStandardItemModel;
class QQmlEngine;
class QJSEngine;

namespace Audio::Internal {

    class AudioPlugin;

    class AudioExporterPresets : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
        Q_PROPERTY(Audio::AudioExporterConfig currentConfig READ currentConfig WRITE setCurrentConfig NOTIFY currentConfigChanged)
        Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)
    public:
        enum Roles {
            NameRole = Qt::UserRole + 1,
            BuiltinRole,
            UnsavedRole,
        };
        Q_ENUM(Roles)

        ~AudioExporterPresets() override;

        static AudioExporterPresets *instance();
        static inline AudioExporterPresets *create(QQmlEngine *, QJSEngine *) {
            return instance();
        }

        QAbstractItemModel *model() const;

        int currentIndex() const;
        void setCurrentIndex(int index);

        Audio::AudioExporterConfig currentConfig() const;
        void setCurrentConfig(const Audio::AudioExporterConfig &config);

        bool isDirty() const;

        Q_INVOKABLE int builtinPresetCount() const;
        Q_INVOKABLE QString builtinPresetName(int builtinIndex) const;
        Q_INVOKABLE Audio::AudioExporterConfig builtinPresetConfig(int builtinIndex) const;
        Q_INVOKABLE bool loadBuiltinPreset(int builtinIndex);

        Q_INVOKABLE QStringList customPresetNames() const;
        Q_INVOKABLE bool hasCustomPreset(const QString &name) const;
        Q_INVOKABLE Audio::AudioExporterConfig customPresetConfig(const QString &name) const;
        Q_INVOKABLE bool loadCustomPreset(const QString &name);

        Q_INVOKABLE bool saveCustomPreset(const QString &name, const Audio::AudioExporterConfig &config);
        Q_INVOKABLE bool saveCurrentAsCustomPreset(const QString &name);
        Q_INVOKABLE bool removeCustomPreset(const QString &name);

        void load();
        void save() const;

    Q_SIGNALS:
        void currentIndexChanged();
        void currentConfigChanged();
        void dirtyChanged();
        void customPresetsChanged();

    private:
        friend class AudioPlugin;
        struct Preset {
            QString name;
            AudioExporterConfig config;
        };

        enum class SelectionType {
            Builtin,
            Custom,
            Unsaved,
        };

        explicit AudioExporterPresets(QObject *parent = nullptr);

        static QString selectionTypeToString(SelectionType selectionType);
        static SelectionType selectionTypeFromString(const QString &value);

        void initializeBuiltinPresets();
        void rebuildModel();
        void updateCurrentIndexFromSelection();
        void setCurrentConfigInternal(const AudioExporterConfig &config, bool dirty);
        [[nodiscard]] int customPresetIndex(const QString &name) const;

        QList<Preset> m_builtinPresets;
        QList<Preset> m_customPresets;
        AudioExporterConfig m_currentConfig;
        bool m_dirty{};
        SelectionType m_selectionType{SelectionType::Builtin};
        int m_currentBuiltinIndex{};
        QString m_currentCustomName;
        QStandardItemModel *m_model{};
        int m_currentIndex{};
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOEXPORTERPRESETS_H
