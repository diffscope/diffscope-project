// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_COLORSCHEMEWELCOMEWIZARDPAGEHELPER_H
#define DIFFSCOPE_COREPLUGIN_COLORSCHEMEWELCOMEWIZARDPAGEHELPER_H

#include <qqmlintegration.h>

#include <QObject>
#include <QVariant>

namespace Core::Internal {

    class ColorSchemeCollection;

    class ColorSchemeWelcomeWizardPageHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QVariantList previewPresets READ previewPresets NOTIFY previewPresetsChanged)
        Q_PROPERTY(int currentPresetIndex READ currentPresetIndex NOTIFY currentPresetIndexChanged)
        Q_PROPERTY(QVariantList accentColors READ accentColors NOTIFY currentPresetIndexChanged)
        Q_PROPERTY(int currentAccentColorIndex READ currentAccentColorIndex NOTIFY currentAccentColorIndexChanged)
    public:
        explicit ColorSchemeWelcomeWizardPageHelper(QObject *parent = nullptr);
        ~ColorSchemeWelcomeWizardPageHelper() override;

        static QVariantList internalPresets();
        QVariantList previewPresets() const;
        Q_INVOKABLE void applyInternalPreset(int index);

        int currentPresetIndex() const;
        QVariantList accentColors() const;
        int currentAccentColorIndex() const;
        Q_INVOKABLE void applyAccentColor(int index);
        Q_INVOKABLE void reload();

    Q_SIGNALS:
        void currentPresetIndexChanged();
        void previewPresetsChanged();
        void currentAccentColorIndexChanged();

    private:
        ColorSchemeCollection *m_colorSchemeCollection{};
        int m_currentPresetIndex{-1};
        int m_currentAccentColorIndex{-1};
    };

} // Core

#endif //DIFFSCOPE_COREPLUGIN_COLORSCHEMEWELCOMEWIZARDPAGEHELPER_H
