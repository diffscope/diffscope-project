#ifndef DIFFSCOPE_COREPLUGIN_COLORSCHEMEPAGE_H
#define DIFFSCOPE_COREPLUGIN_COLORSCHEMEPAGE_H

#include <SVSCraftGui/ColorChange.h>

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class ColorSchemeCollection;

    class ColorSchemePage : public ISettingPage {
        Q_OBJECT
    public:
        explicit ColorSchemePage(QObject *parent = nullptr);
        ~ColorSchemePage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        Q_INVOKABLE static QString colorChangeProperties(const SVS::ColorChange &colorChange);
        Q_INVOKABLE static QString colorChangePropertiesEditText(const SVS::ColorChange &colorChange);
        Q_INVOKABLE static SVS::ColorChange propertiesEditTextToColorChange(const QString &text);

    private:
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        ColorSchemeCollection *m_collection{};
    };
}

#endif //DIFFSCOPE_COREPLUGIN_COLORSCHEMEPAGE_H
