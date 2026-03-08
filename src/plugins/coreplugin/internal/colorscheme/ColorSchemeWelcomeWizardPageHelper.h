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
        Q_PROPERTY(QVariantList internalPresets READ internalPresets CONSTANT)
    public:
        explicit ColorSchemeWelcomeWizardPageHelper(QObject *parent = nullptr);
        ~ColorSchemeWelcomeWizardPageHelper() override;

        static QVariantList internalPresets();
        Q_INVOKABLE void applyInternalPreset(int index) const;

        Q_INVOKABLE int getCurrentPresetIndex() const;

    private:
        ColorSchemeCollection *m_colorSchemeCollection;
    };

} // Core

#endif //DIFFSCOPE_COREPLUGIN_COLORSCHEMEWELCOMEWIZARDPAGEHELPER_H
