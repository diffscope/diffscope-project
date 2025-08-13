#ifndef DIFFSCOPE_COREPLUGIN_COLORSCHEMECOLLECTION_H
#define DIFFSCOPE_COREPLUGIN_COLORSCHEMECOLLECTION_H

#include <QJsonObject>
#include <QObject>
#include <QVariant>
#include <QWindow>

namespace SVS {
    class Theme;
}

namespace sflow {
    class Palette;
}

namespace Core::Internal {

    class ColorSchemeCollection : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantList allPresets READ allPresets NOTIFY allPresetsChanged)
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    public:
        explicit ColorSchemeCollection(QObject *parent = nullptr);
        ~ColorSchemeCollection() override;

        Q_INVOKABLE void setValue(const QString &name, const QVariant &value);
        Q_INVOKABLE QVariant value(const QString &name) const;

        Q_INVOKABLE void loadPreset(int index);
        Q_INVOKABLE void savePreset(const QString &name);
        Q_INVOKABLE void removePreset(int index);
        Q_INVOKABLE void renamePreset(int index, const QString &name);
        Q_INVOKABLE bool presetExists(const QString &name);

        Q_INVOKABLE void importPreset(QWindow *window, const QString &filename);
        Q_INVOKABLE void exportPreset(QWindow *window, const QString &filename) const;

        QVariantList allPresets() const;
        int currentIndex() const;
        void setCurrentIndex(int index);

        void applyTo(SVS::Theme *theme, sflow::Palette *palette) const;
        void load();
        void save() const;

    Q_SIGNALS:
        void unsavedPresetUpdated();
        void allPresetsChanged();
        void currentIndexChanged();

    private:
        QList<QPair<QString, QVariantHash>> m_presets;
        QVariantHash m_unsavedPreset;
        int m_currentIndex{0};
        bool m_showUnsavedPreset{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_COLORSCHEMECOLLECTION_H
