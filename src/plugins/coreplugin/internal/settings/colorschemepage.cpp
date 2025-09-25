#include "colorschemepage.h"

#include <QApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QLoggingCategory>

#include <SVSCraftQuick/Theme.h>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/internal/colorschemecollection.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcColorSchemePage, "diffscope.core.colorscheme")

    ColorSchemePage::ColorSchemePage(QObject *parent) : ISettingPage("core.ColorScheme", parent) {
        m_collection = new ColorSchemeCollection(this);
        setTitle(tr("Color Scheme"));
        setDescription(tr("Configure the colors and visual effects of various components"));
    }

    ColorSchemePage::~ColorSchemePage() {
        delete m_widget;
    }
    bool ColorSchemePage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString ColorSchemePage::sortKeyword() const {
        return QStringLiteral("ColorScheme");
    }

    QObject *ColorSchemePage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcColorSchemePage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ColorSchemePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({
            {"pageHandle", QVariant::fromValue(this)},
            {"collection", QVariant::fromValue(m_collection)},
        });
        m_widget->setParent(this);
        return m_widget;
    }
    void ColorSchemePage::beginSetting() {
        qCInfo(lcColorSchemePage) << "Beginning setting";
        widget();
        m_collection->load();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }
    bool ColorSchemePage::accept() {
        qCInfo(lcColorSchemePage) << "Accepting";
        m_collection->save();
        m_collection->applyTo(SVS::Theme::defaultTheme(), nullptr); // TODO: ScopicFlow editing area palette
        return ISettingPage::accept();
    }

    void ColorSchemePage::endSetting() {
        qCInfo(lcColorSchemePage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    static QString colorToRgba(const QColor &color) {
        return QStringLiteral("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF());
    }

    QString ColorSchemePage::colorChangeProperties(const SVS::ColorChange &colorChange) {
        struct A {
            qintptr data;
            int type;
        };
        QStringList ret;
        for (const auto &filter : colorChange) {
            auto [data, type] = std::bit_cast<A>(filter);
            switch (type) {
                case 0: { // Alpha
                    ret << tr("Alpha: %L1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 1: { // Saturation
                    ret << tr("Saturation (HSV): %L1")
                               .arg(std::bit_cast<double>(data));
                    break;
                }
                case 2: { // Value
                    ret << tr("Value: %L1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 3: { // HslSaturation
                    ret << tr("Saturation (HSL): %L1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 4: { // Lightness
                    ret << tr("Lightness: %L1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 5: { // Lighter
                    ret << tr("QColor::lighter(): %L1").arg(static_cast<int>(data));
                    break;
                }
                case 6: { // TopBlend
                    ret << tr("Top Blend: %1").arg(colorToRgba(QColor::fromRgba(static_cast<QRgb>(data))));
                    break;
                }
                case 7: { // BottomBlend
                    ret << tr("Bottom Blend: %1").arg(colorToRgba(QColor::fromRgba(static_cast<QRgb>(data))));
                    break;
                }
            }
        }
        return ret.join("\n");
    }
    QString ColorSchemePage::colorChangePropertiesEditText(const SVS::ColorChange &colorChange) {
        struct A {
            qintptr data;
            int type;
        };
        QStringList ret;
        for (const auto &filter : colorChange) {
            auto [data, type] = std::bit_cast<A>(filter);
            switch (type) {
                case 0: { // Alpha
                    ret << QStringLiteral("alpha: %1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 1: { // Saturation
                    ret << QStringLiteral("saturation: %1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 2: { // Value
                    ret << QStringLiteral("value: %1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 3: { // HslSaturation
                    ret << QStringLiteral("hsl-saturation: %1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 4: { // Lightness
                    ret << QStringLiteral("lightness: %1").arg(std::bit_cast<double>(data));
                    break;
                }
                case 5: { // Lighter
                    ret << QStringLiteral("lighter: %1").arg(static_cast<int>(data));
                    break;
                }
                case 6: { // TopBlend
                    ret << QStringLiteral("top-blend: %1").arg(colorToRgba(QColor::fromRgba(static_cast<QRgb>(data))));
                    break;
                }
                case 7: { // BottomBlend
                    ret << QStringLiteral("bottom-blend: %1").arg(colorToRgba(QColor::fromRgba(static_cast<QRgb>(data))));
                    break;
                }
            }
        }
        return ret.join("\n");
    }

    static QColor parseColor(const QString &colorStr) {
        QString str = colorStr.trimmed();

        QColor namedColor(str);
        if (namedColor.isValid()) {
            return namedColor;
        }

        QRegularExpression rgbRegex(R"(^rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+)\s*)?\)$)");
        QRegularExpressionMatch match = rgbRegex.match(str);

        if (match.hasMatch()) {
            bool rOk, gOk, bOk;
            int r = match.captured(1).toInt(&rOk);
            int g = match.captured(2).toInt(&gOk);
            int b = match.captured(3).toInt(&bOk);

            if (!rOk || !gOk || !bOk || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                return QColor(); // Invalid RGB values
            }

            if (match.capturedLength(4) > 0) {
                bool aOk;
                double a = match.captured(4).toDouble(&aOk);
                if (!aOk || a < 0.0 || a > 1.0) {
                    return QColor();
                }
                return QColor(r, g, b, static_cast<int>(a * 255));
            } else {
                return QColor(r, g, b);
            }
        }

        return {};
    }

    SVS::ColorChange ColorSchemePage::propertiesEditTextToColorChange(const QString &text) {
        SVS::ColorChange ret;

        QStringList declarations = text.split('\n');
        int lineNumber = 1;
        
        for (const QString &declaration : declarations) {
            QString decl = declaration.trimmed();
            if (decl.isEmpty()) {
                lineNumber++;
                continue;
            }

            int colonIndex = decl.indexOf(':');
            if (colonIndex <= 0) {
                RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Missing colon in declaration").arg(lineNumber));
                return {};
            }
            
            QString propertyName = decl.left(colonIndex).trimmed();
            QString propertyValue = decl.mid(colonIndex + 1).trimmed();
            
            if (propertyName.isEmpty()) {
                RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Empty property name").arg(lineNumber));
                return {};
            }
            
            if (propertyValue.isEmpty()) {
                RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Empty property value").arg(lineNumber));
                return {};
            }

            if (propertyName == "alpha") {
                bool ok;
                double value = propertyValue.toDouble(&ok);
                if (!ok) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'alpha' value").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::AlphaColorFilter(value));
            } else if (propertyName == "saturation") {
                bool ok;
                double value = propertyValue.toDouble(&ok);
                if (!ok) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'saturation' value").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::SaturationColorFilter(value));
            } else if (propertyName == "value") {
                bool ok;
                double value = propertyValue.toDouble(&ok);
                if (!ok) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'value' value").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::ValueColorFilter(value));
            } else if (propertyName == "hsl-saturation") {
                bool ok;
                double value = propertyValue.toDouble(&ok);
                if (!ok) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'hsl-saturation' value").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::HslSaturationColorFilter(value));
            } else if (propertyName == "lightness") {
                bool ok;
                double value = propertyValue.toDouble(&ok);
                if (!ok) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'lightness' value").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::LightnessColorFilter(value));
            } else if (propertyName == "lighter") {
                bool ok;
                int value = propertyValue.toInt(&ok);
                if (!ok || value <= 0) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid 'lighter' value (must be a positive integer)").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::LighterColorChange(value));
            } else if (propertyName == "top-blend") {
                QColor color = parseColor(propertyValue);
                if (!color.isValid()) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid color value for 'top-blend'").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::TopBlendColorFilter(color));
            } else if (propertyName == "bottom-blend") {
                QColor color = parseColor(propertyValue);
                if (!color.isValid()) {
                    RuntimeInterface::qmlEngine()->throwError(tr("Syntax error at line %L1: Invalid color value for 'bottom-blend'").arg(lineNumber));
                    return {};
                }
                ret.append(SVS::BottomBlendColorFilter(color));
            } else {
                RuntimeInterface::qmlEngine()->throwError(
                    tr("Syntax error at line %L1: Unknown property '%2'").arg(lineNumber).arg(propertyName));
                return {};
            }
            
            lineNumber++;
        }
        
        return ret;
    }

    bool ColorSchemePage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }
}
