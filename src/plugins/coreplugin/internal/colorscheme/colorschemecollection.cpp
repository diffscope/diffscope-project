#include "colorschemecollection.h"

#include <algorithm>
#include <ranges>

#include <QColor>

#include <SVSCraftGui/ColorChange.h>
#include <SVSCraftQuick/Theme.h>

namespace Core::Internal {

    static QList<QPair<QString, QVariantHash>> m_internalPresets {
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "Scopic Dark"), {
            {"accentColor", QVariant::fromValue(QColor(0x5566ff))},
            {"warningColor", QVariant::fromValue(QColor(0xCB8743))},
            {"errorColor", QVariant::fromValue(QColor(0xcc4455))},
            {"buttonColor", QVariant::fromValue(QColor(0x333437))},
            {"textFieldColor", QVariant::fromValue(QColor(0x27282b))},
            {"scrollBarColor", QVariant::fromValue(QColor::fromRgba(0x7f7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor(0x4a4b4c))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor(0x212124))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor(0x232427))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor(0x252629))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor(0x313235))},
            {"splitterColor", QVariant::fromValue(QColor(0x121315))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor(0xdadada))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0dadada))},
            {"linkColor", QVariant::fromValue(QColor(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor(0xffffff))},
            {"shadowColor", QVariant::fromValue(QColor(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor(0xb28300))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x33000000)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1affffff)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1affffff)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0x212124}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0x212124}})},
        }},
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "Scopic Light"), {
            {"accentColor", QVariant::fromValue(QColor(0x7d8aff))},
            {"warningColor", QVariant::fromValue(QColor(0xcb9968))},
            {"errorColor", QVariant::fromValue(QColor(0xcc7782))},
            {"buttonColor", QVariant::fromValue(QColor(0xc8cbcc))},
            {"textFieldColor", QVariant::fromValue(QColor(0xd4d7d8))},
            {"scrollBarColor", QVariant::fromValue(QColor::fromRgba(0x7f7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor(0xb3b4b5))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor(0xdbdbde))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor(0xd8dbdc))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor(0xd6d9da))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor(0xcacdce))},
            {"splitterColor", QVariant::fromValue(QColor(0xeaeced))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor(0x252525))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0252525))},
            {"linkColor", QVariant::fromValue(QColor(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor::fromRgb(0x0))},
            {"shadowColor", QVariant::fromValue(QColor(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor(0xb28300))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x33ffffff)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a000000)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a000000)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0xdbdede}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0xdbdede}})},
        }},
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "Scopic High Contrast"), {
            {"accentColor", QVariant::fromValue(QColor(0x0055ff))},
            {"warningColor", QVariant::fromValue(QColor(0xdb5500))},
            {"errorColor", QVariant::fromValue(QColor(0xcc0019))},
            {"buttonColor", QVariant::fromValue(QColor(0x00212b))},
            {"textFieldColor", QVariant::fromValue(QColor(0x00211c))},
            {"scrollBarColor", QVariant::fromValue(QColor(0x7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor(0xbbcc00))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0x000000))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor(0x080808))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor(0x101010))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor(0x181818))},
            {"splitterColor", QVariant::fromValue(QColor(0xcc00aa))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor(0xffffff))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0ffffff))},
            {"linkColor", QVariant::fromValue(QColor(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor::fromRgb(0xcc00aa))},
            {"shadowColor", QVariant::fromValue(QColor(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor(0xb28300))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x3300c4ff)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a00c4ff)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a00c4ff)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0x212124}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0x212124}})},
        }}
    };

    ColorSchemeCollection::ColorSchemeCollection(QObject *parent) : QObject(parent), m_unsavedPreset(m_internalPresets[0].second) {
    }
    ColorSchemeCollection::~ColorSchemeCollection() = default;
    void ColorSchemeCollection::setValue(const QString &name, const QVariant &value) {
        if (m_unsavedPreset.contains(name) && m_unsavedPreset.value(name) == value) {
            return;
        }
        m_unsavedPreset.insert(name, value);
        if (!m_showUnsavedPreset) {
            m_showUnsavedPreset = true;
            m_currentIndex = m_internalPresets.size() + m_presets.size();
            emit allPresetsChanged();
            emit currentIndexChanged();
        }
        emit unsavedPresetUpdated();
    }
    QVariant ColorSchemeCollection::value(const QString &name) const {
        return m_unsavedPreset.value(name);
    }
    void ColorSchemeCollection::loadPreset(int index) {
        QVariantHash preset;
        if (index < m_internalPresets.size()) {
            preset = m_internalPresets.at(index).second;
        } else if (index < m_internalPresets.size() + m_presets.size()) {
            preset = m_presets.at(index - m_internalPresets.size()).second;
        } else {
            return;
        }
        m_unsavedPreset = preset;
        m_currentIndex = index;
        m_showUnsavedPreset = false;
        emit allPresetsChanged();
        emit currentIndexChanged();
        emit unsavedPresetUpdated();
    }
    void ColorSchemeCollection::savePreset(const QString &name) {
        if (auto it = std::ranges::find_if(m_presets, [&](const auto &p) { return p.first == name; }); it != m_presets.end()) {
            it->second = m_unsavedPreset;
            m_currentIndex = m_internalPresets.size() + std::distance(m_presets.begin(), it);
            m_showUnsavedPreset = false;
            emit allPresetsChanged();
            emit currentIndexChanged();
        } else {
            m_presets.append({name, m_unsavedPreset});
            m_currentIndex = m_internalPresets.size() + m_presets.size() - 1;
            m_showUnsavedPreset = false;
            emit allPresetsChanged();
            emit currentIndexChanged();
        }
    }
    void ColorSchemeCollection::removePreset(int index) {
        if (index < m_internalPresets.size() || index >= m_internalPresets.size() + m_presets.size()) {
            return;
        }
        m_presets.removeAt(index - m_internalPresets.size());
        if (index < m_currentIndex) {
            m_currentIndex = qMax(0, m_currentIndex - 1);
            emit currentIndexChanged();
        } else if (index == m_currentIndex) {
            m_showUnsavedPreset = true;
            m_currentIndex = m_internalPresets.size() + m_presets.size();
            emit currentIndexChanged();
        }
        emit allPresetsChanged();
    }
    void ColorSchemeCollection::renamePreset(int index, const QString &name) {
        if (index < m_internalPresets.size() || index >= m_internalPresets.size() + m_presets.size()) {
            return;
        }
        index -= m_internalPresets.size();
        if (m_presets.value(index).first == name) {
            return;
        }
        m_presets[index].first = name;
        emit allPresetsChanged();
    }
    bool ColorSchemeCollection::presetExists(const QString &name) {
        return std::ranges::any_of(m_presets, [&](const auto &p) { return p.first == name; });
    }
    void ColorSchemeCollection::importPreset(const QJsonObject &json) {
    }
    QJsonObject ColorSchemeCollection::exportPreset(const QString &name) const {
        return {};
    }
    QVariantList ColorSchemeCollection::allPresets() const {
        QVariantList ret;
        std::ranges::transform(
            m_internalPresets | std::views::transform([this](const auto &p) {
                return qMakePair(tr(p.first.toUtf8()), p.second);
            }),
            std::back_inserter(ret),
            [](const QPair<QString, QVariantHash> &p) -> QVariant {
                return QVariantMap {
                    {"name", p.first},
                    {"internal", true},
                };
            }
        );
        std::ranges::transform(
            m_presets,
            std::back_inserter(ret),
            [](const QPair<QString, QVariantHash> &p) -> QVariant {
                return QVariantMap {
                    {"name", p.first},
                    {"internal", false},
                };
            }
        );
        if (m_showUnsavedPreset) {
            ret.append(QVariantMap {
                {"name", tr("(Unsaved preset)")},
                {"internal", true},
            });
        }
        return ret;
    }
    int ColorSchemeCollection::currentIndex() const {
        return m_currentIndex;
    }
    void ColorSchemeCollection::setCurrentIndex(int index) {
        if (m_currentIndex != index) {
            m_currentIndex = index;
            emit currentIndexChanged();
        }
    }
    void ColorSchemeCollection::applyTo(SVS::Theme *theme, sflow::Palette *palette) const {
        theme->setProperty("accentColor", m_unsavedPreset.value("accentColor"));
        theme->setProperty("warningColor", m_unsavedPreset.value("warningColor"));
        theme->setProperty("errorColor", m_unsavedPreset.value("errorColor"));
        theme->setProperty("buttonColor", m_unsavedPreset.value("buttonColor"));
        theme->setProperty("textFieldColor", m_unsavedPreset.value("textFieldColor"));
        theme->setProperty("scrollBarColor", m_unsavedPreset.value("scrollBarColor"));
        theme->setProperty("borderColor", m_unsavedPreset.value("borderColor"));
        theme->setProperty("backgroundPrimaryColor", m_unsavedPreset.value("backgroundPrimaryColor"));
        theme->setProperty("backgroundSecondaryColor", m_unsavedPreset.value("backgroundSecondaryColor"));
        theme->setProperty("backgroundTertiaryColor", m_unsavedPreset.value("backgroundTertiaryColor"));
        theme->setProperty("backgroundQuaternaryColor", m_unsavedPreset.value("backgroundQuaternaryColor"));
        theme->setProperty("splitterColor", m_unsavedPreset.value("splitterColor"));
        theme->setProperty("foregroundPrimaryColor", m_unsavedPreset.value("foregroundPrimaryColor"));
        theme->setProperty("foregroundSecondaryColor", m_unsavedPreset.value("foregroundSecondaryColor"));
        theme->setProperty("linkColor", m_unsavedPreset.value("linkColor"));
        theme->setProperty("navigationColor", m_unsavedPreset.value("navigationColor"));
        theme->setProperty("shadowColor", m_unsavedPreset.value("shadowColor"));
        theme->setProperty("highlightColor", m_unsavedPreset.value("highlightColor"));
        theme->setProperty("controlDisabledColorChange", m_unsavedPreset.value("controlDisabledColorChange"));
        theme->setProperty("foregroundDisabledColorChange", m_unsavedPreset.value("foregroundDisabledColorChange"));
        theme->setProperty("controlHoveredColorChange", m_unsavedPreset.value("controlHoveredColorChange"));
        theme->setProperty("foregroundHoveredColorChange", m_unsavedPreset.value("foregroundHoveredColorChange"));
        theme->setProperty("controlPressedColorChange", m_unsavedPreset.value("controlPressedColorChange"));
        theme->setProperty("foregroundPressedColorChange", m_unsavedPreset.value("foregroundPressedColorChange"));
        theme->setProperty("controlCheckedColorChange", m_unsavedPreset.value("controlCheckedColorChange"));
        theme->setProperty("annotationPopupTitleColorChange", m_unsavedPreset.value("annotationPopupTitleColorChange"));
        theme->setProperty("annotationPopupContentColorChange", m_unsavedPreset.value("annotationPopupContentColorChange"));
    }
    void ColorSchemeCollection::load() {
    }
    void ColorSchemeCollection::save() const {

    }
}