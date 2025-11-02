#include "ColorSchemeWelcomeWizardPageHelper.h"

#include <algorithm>

#include <SVSCraftQuick/Theme.h>

#include <coreplugin/internal/ColorSchemeCollection.h>

namespace Core::Internal {
    ColorSchemeWelcomeWizardPageHelper::ColorSchemeWelcomeWizardPageHelper(QObject *parent) : QObject(parent) {
        m_colorSchemeCollection = new ColorSchemeCollection(this);
        m_colorSchemeCollection->load();
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

    void ColorSchemeWelcomeWizardPageHelper::applyInternalPreset(int index) const {
        m_colorSchemeCollection->setCurrentIndex(index);
        m_colorSchemeCollection->loadPreset(index);
        m_colorSchemeCollection->save();
        m_colorSchemeCollection->applyTo(SVS::Theme::defaultTheme(), nullptr); // TODO: ScopicFlow editing area palette
    }

    int ColorSchemeWelcomeWizardPageHelper::getCurrentPresetIndex() const {
        return m_colorSchemeCollection->currentIndex();
    }
} // Core
