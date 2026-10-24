// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ColorSchemeWelcomeWizardPageHelper.h"

#include <algorithm>
#include <iterator>

#include <QColor>

#include <SVSCraftGui/ColorChange.h>

#include <coreplugin/internal/ColorSchemeCollection.h>

namespace Core::Internal {
    ColorSchemeWelcomeWizardPageHelper::ColorSchemeWelcomeWizardPageHelper(QObject *parent) : QObject(parent) {
        m_colorSchemeCollection = new ColorSchemeCollection(this);
        reload();
    }

    ColorSchemeWelcomeWizardPageHelper::~ColorSchemeWelcomeWizardPageHelper() = default;

    QVariantList ColorSchemeWelcomeWizardPageHelper::internalPresets() {
        static QVariantList presets;
        if (presets.isEmpty()) {
            auto a = ColorSchemeCollection::internalPresets();
            std::ranges::transform(a, std::back_inserter(presets), [](const auto &p) {
                return p.second;
            });
        }
        return presets;
    }

    QVariantList ColorSchemeWelcomeWizardPageHelper::previewPresets() const {
        auto presets = internalPresets();
        if (m_currentPresetIndex >= 0) {
            auto preset = presets.at(m_currentPresetIndex).toHash();
            for (auto it = preset.begin(); it != preset.end(); ++it) {
                it.value() = m_colorSchemeCollection->value(it.key());
            }
            presets[m_currentPresetIndex] = preset;
        }
        return presets;
    }

    void ColorSchemeWelcomeWizardPageHelper::applyInternalPreset(int index) {
        if (index < 0 || index >= internalPresets().size()) {
            return;
        }
        m_colorSchemeCollection->loadPreset(index);
        m_colorSchemeCollection->save();
        m_colorSchemeCollection->apply();
        m_currentPresetIndex = index;
        m_currentAccentColorIndex = 0;
        Q_EMIT currentPresetIndexChanged();
        Q_EMIT currentAccentColorIndexChanged();
        Q_EMIT previewPresetsChanged();
    }

    int ColorSchemeWelcomeWizardPageHelper::currentPresetIndex() const {
        return m_currentPresetIndex;
    }

    QVariantList ColorSchemeWelcomeWizardPageHelper::accentColors() const {
        const auto preset = internalPresets().at(qMax(0, m_currentPresetIndex)).toHash();
        QVariantList colors;
        for (int i = 0; i < 12; ++i) {
            colors.append(preset.value(QStringLiteral("trackColorSchema%1").arg(i)));
        }
        return colors;
    }

    int ColorSchemeWelcomeWizardPageHelper::currentAccentColorIndex() const {
        return m_currentAccentColorIndex;
    }

    void ColorSchemeWelcomeWizardPageHelper::applyAccentColor(int index) {
        if (m_currentPresetIndex < 0 || index < 0 || index >= 12 || index == m_currentAccentColorIndex) {
            return;
        }
        if (index == 0) {
            applyInternalPreset(m_currentPresetIndex);
            return;
        }

        const auto preset = internalPresets().at(m_currentPresetIndex).toHash();
        const auto accentColor = preset.value(QStringLiteral("trackColorSchema%1").arg(index)).value<QColor>();

        m_colorSchemeCollection->loadPreset(m_currentPresetIndex);
        m_colorSchemeCollection->setValue("accentColor", accentColor);
        m_colorSchemeCollection->setValue("dockingPanelHeaderActiveColorChange", QVariant::fromValue(ColorSchemeCollection::internalPresetHeaderColorChange(m_currentPresetIndex, accentColor)));
        for (int i = 0; i < 12; ++i) {
            m_colorSchemeCollection->setValue(QStringLiteral("trackColorSchema%1").arg(i), preset.value(QStringLiteral("trackColorSchema%1").arg((i + index) % 12)));
        }
        m_colorSchemeCollection->save();
        m_colorSchemeCollection->apply();
        m_currentAccentColorIndex = index;
        Q_EMIT currentAccentColorIndexChanged();
        Q_EMIT previewPresetsChanged();
    }

    void ColorSchemeWelcomeWizardPageHelper::reload() {
        m_colorSchemeCollection->load();
        const auto index = m_colorSchemeCollection->currentIndex();
        m_currentPresetIndex = index >= 0 && index < internalPresets().size() ? index : -1;
        m_currentAccentColorIndex = m_currentPresetIndex >= 0 ? 0 : -1;
        Q_EMIT currentPresetIndexChanged();
        Q_EMIT currentAccentColorIndexChanged();
        Q_EMIT previewPresetsChanged();
    }
} // Core
