#include "ColorSchemeCollection.h"

#include <algorithm>
#include <ranges>

#include <QColor>
#include <QFile>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftGui/ColorChange.h>
#include <SVSCraftQuick/MessageBox.h>
#include <SVSCraftQuick/Theme.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/TrackColorSchema.h>

namespace Core::Internal {

    static QList<QPair<QString, QVariantHash>> m_internalPresets{
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "DiffScope Dark"),
         {
            {"accentColor", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"warningColor", QVariant::fromValue(QColor::fromRgb(0xCB8743))},
            {"errorColor", QVariant::fromValue(QColor::fromRgb(0xcc4455))},
            {"buttonColor", QVariant::fromValue(QColor::fromRgb(0x333437))},
            {"textFieldColor", QVariant::fromValue(QColor::fromRgb(0x27282b))},
            {"scrollBarColor", QVariant::fromValue(QColor::fromRgba(0x7f7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor::fromRgb(0x4a4b4c))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0x212124))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor::fromRgb(0x232427))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor::fromRgb(0x252629))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor::fromRgb(0x313235))},
            {"splitterColor", QVariant::fromValue(QColor::fromRgb(0x121315))},
            {"paneSeparatorColor", QVariant::fromValue(QColor::fromRgb(0x343538))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0xdadada))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0dadada))},
            {"linkColor", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor::fromRgb(0xffffff))},
            {"shadowColor", QVariant::fromValue(QColor::fromRgb(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor::fromRgb(0xb28300))},
            {"flatButtonHighContrastBorderColor", QVariant::fromValue(QColor(Qt::transparent))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x33000000)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1affffff)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1affffff)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0x212124}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0x212124}})},
            {"dockingPanelHeaderActiveColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x265566ff)}})},
            {"trackColorSchema0", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"trackColorSchema1", QVariant::fromValue(QColor::fromRgb(0x8f50ef))},
            {"trackColorSchema2", QVariant::fromValue(QColor::fromRgb(0xb641c3))},
            {"trackColorSchema3", QVariant::fromValue(QColor::fromRgb(0xc64291))},
            {"trackColorSchema4", QVariant::fromValue(QColor::fromRgb(0xcf4553))},
            {"trackColorSchema5", QVariant::fromValue(QColor::fromRgb(0xae693a))},
            {"trackColorSchema6", QVariant::fromValue(QColor::fromRgb(0x887f2d))},
            {"trackColorSchema7", QVariant::fromValue(QColor::fromRgb(0x668a2e))},
            {"trackColorSchema8", QVariant::fromValue(QColor::fromRgb(0x3b9331))},
            {"trackColorSchema9", QVariant::fromValue(QColor::fromRgb(0x319257))},
            {"trackColorSchema10", QVariant::fromValue(QColor::fromRgb(0x2f8d84))},
            {"trackColorSchema11", QVariant::fromValue(QColor::fromRgb(0x3c84b4))},
            {"loopColor", QVariant::fromValue(QColor(0xcc7f00))},
            {"levelMeterColor", QVariant::fromValue(QColor(0x111112))},
            {"editAreaPrimaryColor", QVariant::fromValue(QColor(0x333740))},
            {"editAreaSecondaryColor", QVariant::fromValue(QColor(0x444954))},
            {"playheadPrimaryColor", QVariant::fromValue(QColor(0xcc4455))},
            {"playheadSecondaryColor", QVariant::fromValue(QColor::fromRgba(0x7fcc4455))},
            {"cursorIndicatorColor", QVariant::fromValue(QColor::fromRgba(0xbf00bfff))},
            {"scissorIndicatorColor", QVariant::fromValue(QColor(0xcc4455))},
            {"scalePrimaryColor", QVariant::fromValue(QColor(Qt::black))},
            {"scaleSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xbf000000))},
            {"scaleTertiaryColor", QVariant::fromValue(QColor::fromRgba(0x7f000000))},
            {"levelLowColor", QVariant::fromValue(QColor(0x00c853))},
            {"levelMediumColor", QVariant::fromValue(QColor(0xffab00))},
            {"levelHighColor", QVariant::fromValue(QColor(0xff2c00))},
            {"muteColor", QVariant::fromValue(QColor(0xe67700))},
            {"soloColor", QVariant::fromValue(QColor(0x2b8a3e))},
            {"recordColor", QVariant::fromValue(QColor(0xc92a2a))},
            {"routeColor", QVariant::fromValue(QColor(0x5f3dc4))},
            {"clipMuteColor", QVariant::fromValue(QColor(0x7c7c7c))},
            {"whiteKeyColor", QVariant::fromValue(QColor(0xf8f9fa))},
            {"blackKeyColor", QVariant::fromValue(QColor(0x212529))},
            {"whiteKeyTextColor", QVariant::fromValue(QColor(0x252525))},
            {"blackKeyTextColor", QVariant::fromValue(QColor(0xdadada))},
            {"itemSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x11ffffff)}})},
            {"clipSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
            {"clipThumbnailColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"noteSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
         }},
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "DiffScope Light"),
         {
            {"accentColor", QVariant::fromValue(QColor::fromRgb(0x7d8aff))},
            {"warningColor", QVariant::fromValue(QColor::fromRgb(0xcb9968))},
            {"errorColor", QVariant::fromValue(QColor::fromRgb(0xcc7782))},
            {"buttonColor", QVariant::fromValue(QColor::fromRgb(0xc8cbcc))},
            {"textFieldColor", QVariant::fromValue(QColor::fromRgb(0xd4d7d8))},
            {"scrollBarColor", QVariant::fromValue(QColor::fromRgba(0x7f7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor::fromRgb(0xb3b4b5))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0xdbdbde))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor::fromRgb(0xd8dbdc))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor::fromRgb(0xd6d9da))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor::fromRgb(0xcacdce))},
            {"splitterColor", QVariant::fromValue(QColor::fromRgb(0xeaeced))},
            {"paneSeparatorColor", QVariant::fromValue(QColor::fromRgb(0xc7cacb))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0x252525))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0252525))},
            {"linkColor", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor::fromRgb(0x0))},
            {"shadowColor", QVariant::fromValue(QColor::fromRgb(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor::fromRgb(0xb28300))},
            {"flatButtonHighContrastBorderColor", QVariant::fromValue(QColor(Qt::transparent))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x33ffffff)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a000000)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a000000)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0xdbdede}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0xdbdede}})},
            {"dockingPanelHeaderActiveColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x607d8aff)}})},
            {"trackColorSchema0", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"trackColorSchema1", QVariant::fromValue(QColor::fromRgb(0x8f50ef))},
            {"trackColorSchema2", QVariant::fromValue(QColor::fromRgb(0xb641c3))},
            {"trackColorSchema3", QVariant::fromValue(QColor::fromRgb(0xc64291))},
            {"trackColorSchema4", QVariant::fromValue(QColor::fromRgb(0xcf4553))},
            {"trackColorSchema5", QVariant::fromValue(QColor::fromRgb(0xae693a))},
            {"trackColorSchema6", QVariant::fromValue(QColor::fromRgb(0x887f2d))},
            {"trackColorSchema7", QVariant::fromValue(QColor::fromRgb(0x668a2e))},
            {"trackColorSchema8", QVariant::fromValue(QColor::fromRgb(0x3b9331))},
            {"trackColorSchema9", QVariant::fromValue(QColor::fromRgb(0x319257))},
            {"trackColorSchema10", QVariant::fromValue(QColor::fromRgb(0x2f8d84))},
            {"trackColorSchema11", QVariant::fromValue(QColor::fromRgb(0x3c84b4))},
            {"loopColor", QVariant::fromValue(QColor(0xcc7f00))},
            {"levelMeterColor", QVariant::fromValue(QColor(0x111112))},
            {"editAreaPrimaryColor", QVariant::fromValue(QColor(0x333740))},
            {"editAreaSecondaryColor", QVariant::fromValue(QColor(0x444954))},
            {"playheadPrimaryColor", QVariant::fromValue(QColor(0xcc4455))},
            {"playheadSecondaryColor", QVariant::fromValue(QColor::fromRgba(0x7fcc4455))},
            {"cursorIndicatorColor", QVariant::fromValue(QColor::fromRgba(0x7f5566ff))},
            {"scissorIndicatorColor", QVariant::fromValue(QColor(0xcc4455))},
            {"scalePrimaryColor", QVariant::fromValue(QColor(Qt::black))},
            {"scaleSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xbf000000))},
            {"scaleTertiaryColor", QVariant::fromValue(QColor::fromRgba(0x7f000000))},
            {"levelLowColor", QVariant::fromValue(QColor(0x00c853))},
            {"levelMediumColor", QVariant::fromValue(QColor(0xffab00))},
            {"levelHighColor", QVariant::fromValue(QColor(0xff2c00))},
            {"muteColor", QVariant::fromValue(QColor(0xe67700))},
            {"soloColor", QVariant::fromValue(QColor(0x2b8a3e))},
            {"recordColor", QVariant::fromValue(QColor(0xc92a2a))},
            {"routeColor", QVariant::fromValue(QColor(0x5f3dc4))},
            {"clipMuteColor", QVariant::fromValue(QColor(0x7c7c7c))},
            {"whiteKeyColor", QVariant::fromValue(QColor(0xf8f9fa))},
            {"blackKeyColor", QVariant::fromValue(QColor(0x212529))},
            {"whiteKeyTextColor", QVariant::fromValue(QColor(0x252525))},
            {"blackKeyTextColor", QVariant::fromValue(QColor(0xdadada))},
            {"itemSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x11ffffff)}})},
            {"clipSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
            {"clipThumbnailColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"noteSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
         }},
        {QT_TRANSLATE_NOOP("Core::Internal::ColorSchemeCollection", "DiffScope High Contrast"),
         {
            {"accentColor", QVariant::fromValue(QColor::fromRgb(0x0055ff))},
            {"warningColor", QVariant::fromValue(QColor::fromRgb(0xdb5500))},
            {"errorColor", QVariant::fromValue(QColor::fromRgb(0xcc0019))},
            {"buttonColor", QVariant::fromValue(QColor::fromRgb(0x00212b))},
            {"textFieldColor", QVariant::fromValue(QColor::fromRgb(0x00211c))},
            {"scrollBarColor", QVariant::fromValue(QColor::fromRgb(0x7f7f7f))},
            {"borderColor", QVariant::fromValue(QColor::fromRgb(0xbbcc00))},
            {"backgroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0x000000))},
            {"backgroundSecondaryColor", QVariant::fromValue(QColor::fromRgb(0x060606))},
            {"backgroundTertiaryColor", QVariant::fromValue(QColor::fromRgb(0x0c0c0c))},
            {"backgroundQuaternaryColor", QVariant::fromValue(QColor::fromRgb(0x121212))},
            {"splitterColor", QVariant::fromValue(QColor::fromRgb(0x00aacc))},
            {"paneSeparatorColor", QVariant::fromValue(QColor::fromRgb(0x7f7f7f))},
            {"foregroundPrimaryColor", QVariant::fromValue(QColor::fromRgb(0xffffff))},
            {"foregroundSecondaryColor", QVariant::fromValue(QColor::fromRgba(0xa0ffffff))},
            {"linkColor", QVariant::fromValue(QColor::fromRgb(0x5566ff))},
            {"navigationColor", QVariant::fromValue(QColor::fromRgb(0xcc00aa))},
            {"shadowColor", QVariant::fromValue(QColor::fromRgb(0x101113))},
            {"highlightColor", QVariant::fromValue(QColor::fromRgb(0xb28300))},
            {"flatButtonHighContrastBorderColor", QVariant::fromValue(QColor::fromRgb(0x00db58))},
            {"controlDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x33000000)}})},
            {"foregroundDisabledColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"controlHoveredColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a00c4ff)}})},
            {"foregroundHoveredColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"controlPressedColorChange", QVariant::fromValue(SVS::ColorChange{})},
            {"foregroundPressedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.8}})},
            {"controlCheckedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x1a00c4ff)}})},
            {"annotationPopupTitleColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.72}, SVS::BottomBlendColorFilter{0x212124}})},
            {"annotationPopupContentColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.16}, SVS::BottomBlendColorFilter{0x212124}})},
            {"dockingPanelHeaderActiveColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x400055ff)}})},
            {"trackColorSchema0", QVariant::fromValue(QColor::fromRgb(0x0055ff))},
            {"trackColorSchema1", QVariant::fromValue(QColor::fromRgb(0x2a00ff))},
            {"trackColorSchema2", QVariant::fromValue(QColor::fromRgb(0x9700e3))},
            {"trackColorSchema3", QVariant::fromValue(QColor::fromRgb(0xb9009a))},
            {"trackColorSchema4", QVariant::fromValue(QColor::fromRgb(0xca0043))},
            {"trackColorSchema5", QVariant::fromValue(QColor::fromRgb(0xc72100))},
            {"trackColorSchema6", QVariant::fromValue(QColor::fromRgb(0x926100))},
            {"trackColorSchema7", QVariant::fromValue(QColor::fromRgb(0x637700))},
            {"trackColorSchema8", QVariant::fromValue(QColor::fromRgb(0x2b8200))},
            {"trackColorSchema9", QVariant::fromValue(QColor::fromRgb(0x008416))},
            {"trackColorSchema10", QVariant::fromValue(QColor::fromRgb(0x008156))},
            {"trackColorSchema11", QVariant::fromValue(QColor::fromRgb(0x007992))},
            {"loopColor", QVariant::fromValue(QColor::fromRgb(0xcc7f00))},
            {"levelMeterColor", QVariant::fromValue(QColor::fromRgb(0x111112))},
            {"editAreaPrimaryColor", QVariant::fromValue(QColor::fromRgb(0x292c33))},
            {"editAreaSecondaryColor", QVariant::fromValue(QColor::fromRgb(0x535966))},
            {"playheadPrimaryColor", QVariant::fromValue(QColor::fromRgb(0xcc293c))},
            {"playheadSecondaryColor", QVariant::fromValue(QColor::fromRgba(0x7fcc293c))},
            {"cursorIndicatorColor", QVariant::fromValue(QColor::fromRgba(0xbf00ff55))},
            {"scissorIndicatorColor", QVariant::fromValue(QColor(0xcc293c))},
            {"scalePrimaryColor", QVariant::fromValue(QColor::fromRgb(0x007fff))},
            {"scaleSecondaryColor", QVariant::fromValue(QColor::fromRgb(0x005fbf))},
            {"scaleTertiaryColor", QVariant::fromValue(QColor::fromRgb(0x003f7f))},
            {"levelLowColor", QVariant::fromValue(QColor::fromRgb(0x00c853))},
            {"levelMediumColor", QVariant::fromValue(QColor::fromRgb(0xffab00))},
            {"levelHighColor", QVariant::fromValue(QColor::fromRgb(0xff2c00))},
            {"muteColor", QVariant::fromValue(QColor::fromRgb(0xe67700))},
            {"soloColor", QVariant::fromValue(QColor::fromRgb(0x2b8a3e))},
            {"recordColor", QVariant::fromValue(QColor::fromRgb(0xc92a2a))},
            {"routeColor", QVariant::fromValue(QColor::fromRgb(0x5f3dc4))},
            {"clipMuteColor", QVariant::fromValue(QColor::fromRgb(0x6d6d6d))},
            {"whiteKeyColor", QVariant::fromValue(QColor::fromRgb(0x6699cc))},
            {"blackKeyColor", QVariant::fromValue(QColor::fromRgb(0x000000))},
            {"whiteKeyTextColor", QVariant::fromValue(QColor::fromRgb(0x000000))},
            {"blackKeyTextColor", QVariant::fromValue(QColor(0xffffff))},
            {"itemSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::TopBlendColorFilter{QColor::fromRgba(0x11ffffff)}})},
            {"clipSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
            {"clipThumbnailColorChange", QVariant::fromValue(SVS::ColorChange{SVS::AlphaColorFilter{0.5}})},
            {"noteSelectedColorChange", QVariant::fromValue(SVS::ColorChange{SVS::OkLabLighterColorChange{1.2}})},
         }}
    };

    static const int MAX_INTERNAL_PRESETS = 65536;

    static QByteArray serializePreset(const QVariantHash &preset) {
        QByteArray a;
        QDataStream stream(&a, QDataStream::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_9);
        stream << preset;
        return a;
    }
    static QVariantHash deserializePreset(const QByteArray &data) {
        QVariantHash preset;
        QDataStream stream(data);
        stream.setVersion(QDataStream::Qt_6_9);
        stream >> preset;
        if (preset.isEmpty()) {
            return preset;
        }
        for (const auto &key : m_internalPresets[0].second.keys()) {
            if (!preset.contains(key)) {
                preset[key] = m_internalPresets[0].second[key];
            }
        }
        return preset;
    }

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
            m_currentIndex = MAX_INTERNAL_PRESETS + m_presets.size();
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
        if (index < MAX_INTERNAL_PRESETS) {
            preset = m_internalPresets.value(index, m_internalPresets.first()).second;
        } else if (index < MAX_INTERNAL_PRESETS + m_presets.size()) {
            preset = m_presets.at(index - MAX_INTERNAL_PRESETS).second;
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
            m_currentIndex = MAX_INTERNAL_PRESETS + std::distance(m_presets.begin(), it);
            m_showUnsavedPreset = false;
            emit allPresetsChanged();
            emit currentIndexChanged();
        } else {
            m_presets.append({name, m_unsavedPreset});
            m_currentIndex = MAX_INTERNAL_PRESETS + m_presets.size() - 1;
            m_showUnsavedPreset = false;
            emit allPresetsChanged();
            emit currentIndexChanged();
        }
    }
    void ColorSchemeCollection::removePreset(int index) {
        if (index < MAX_INTERNAL_PRESETS || index >= MAX_INTERNAL_PRESETS + m_presets.size()) {
            return;
        }
        m_presets.removeAt(index - MAX_INTERNAL_PRESETS);
        if (index < m_currentIndex) {
            m_currentIndex = qMax(0, m_currentIndex - 1);
        } else if (index == m_currentIndex) {
            m_showUnsavedPreset = true;
            m_currentIndex = MAX_INTERNAL_PRESETS + m_presets.size();
        }
        emit allPresetsChanged();
        emit currentIndexChanged();
    }
    void ColorSchemeCollection::renamePreset(int index, const QString &name) {
        if (index < MAX_INTERNAL_PRESETS || index >= MAX_INTERNAL_PRESETS + m_presets.size()) {
            return;
        }
        index -= MAX_INTERNAL_PRESETS;
        auto originName = m_presets.value(index).first;
        m_presets.removeIf([&](const auto &o) { return o.first == name; });
        auto it = std::ranges::find_if(m_presets, [&](const auto &o) { return o.first == originName; });
        Q_ASSERT(it != m_presets.end());
        it->first = name;
        emit allPresetsChanged();
        index = MAX_INTERNAL_PRESETS + std::distance(m_presets.begin(), it);
        m_currentIndex = index;
        emit currentIndexChanged();
    }
    bool ColorSchemeCollection::presetExists(const QString &name) {
        return std::ranges::any_of(m_presets, [&](const auto &p) { return p.first == name; });
    }
    void ColorSchemeCollection::importPreset(QWindow *window, const QUrl &fileUrl) {
        auto filename = fileUrl.toLocalFile();
        QFile f(filename);
        if (!f.open(QIODevice::ReadOnly)) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), window, tr("Failed to Import Preset"), tr("Unable to open file \"%1\"").arg(filename));
            return;
        }
        QDataStream in(&f);
        in.setVersion(QDataStream::Qt_6_9);
        QString name;
        in >> name;
        QByteArray a;
        in >> a;
        if (name.isEmpty() || a.isEmpty()) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), window, tr("Failed to import preset"), tr("Invalid format in file \"%1\"").arg(filename));
            return;
        }
        auto preset = deserializePreset(a);
        m_unsavedPreset = preset;
        savePreset(name);
    }
    void ColorSchemeCollection::exportPreset(QWindow *window, const QUrl &fileUrl) const {
        auto filename = fileUrl.toLocalFile();
        if (m_currentIndex >= MAX_INTERNAL_PRESETS + m_presets.size()) {
            return;
        }
        const auto &[name, preset] = m_currentIndex < MAX_INTERNAL_PRESETS ? m_internalPresets.value(m_currentIndex, m_internalPresets.first()) : m_presets.value(m_currentIndex - MAX_INTERNAL_PRESETS);
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly)) {
            SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), window, tr("Failed to export preset"), tr("Unable to open file \"%1\"").arg(filename));
            return;
        }
        QDataStream out(&f);
        out.setVersion(QDataStream::Qt_6_9);
        out << name;
        out << serializePreset(preset);
    }
    QVariantList ColorSchemeCollection::allPresets() const {
        QVariantList ret;
        std::ranges::transform(
            m_internalPresets | std::views::transform([this](const auto &p) {
                return qMakePair(tr(p.first.toUtf8()), p.second);
            }),
            std::back_inserter(ret),
            [](const QPair<QString, QVariantHash> &p) -> QVariant {
                return QVariantMap{
                    {"name", p.first},
                    {"data", QVariantMap{
                                 {"internal", true},
                                 {"indexOffset", 0}
                             }}

                };
            }
        );
        std::ranges::transform(
            m_presets,
            std::back_inserter(ret),
            [](const QPair<QString, QVariantHash> &p) -> QVariant {
                return QVariantMap{
                    {"name", p.first},
                    {"data", QVariantMap{
                                 {"internal", false},
                                 {"indexOffset", MAX_INTERNAL_PRESETS - m_internalPresets.size()},
                             }}
                };
            }
        );
        if (m_showUnsavedPreset) {
            ret.append(QVariantMap{
                {"name", tr("(Unsaved preset)")},
                {"data", QVariantMap{
                             {"internal", true},
                             {"unsaved", true},
                             {"indexOffset", MAX_INTERNAL_PRESETS - m_internalPresets.size()},
                         }}
            });
        }
        return ret;
    }
    int ColorSchemeCollection::currentIndex() const {
        return m_currentIndex;
    }
    int ColorSchemeCollection::visualCurrentIndex() const {
        if (m_currentIndex < m_internalPresets.size()) {
            return m_currentIndex;
        } else if (m_currentIndex < MAX_INTERNAL_PRESETS) {
            return 0;
        } else {
            return m_currentIndex - MAX_INTERNAL_PRESETS + m_internalPresets.size();
        }
    }
    void ColorSchemeCollection::setCurrentIndex(int index) {
        if (m_currentIndex != index) {
            m_currentIndex = index;
            emit currentIndexChanged();
        }
    }
    void ColorSchemeCollection::apply() const {
        auto theme = SVS::Theme::defaultTheme();
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
        theme->setProperty("paneSeparatorColor", m_unsavedPreset.value("paneSeparatorColor"));
        theme->setProperty("foregroundPrimaryColor", m_unsavedPreset.value("foregroundPrimaryColor"));
        theme->setProperty("foregroundSecondaryColor", m_unsavedPreset.value("foregroundSecondaryColor"));
        theme->setProperty("linkColor", m_unsavedPreset.value("linkColor"));
        theme->setProperty("navigationColor", m_unsavedPreset.value("navigationColor"));
        theme->setProperty("shadowColor", m_unsavedPreset.value("shadowColor"));
        theme->setProperty("highlightColor", m_unsavedPreset.value("highlightColor"));
        theme->setProperty("flatButtonHighContrastBorderColor", m_unsavedPreset.value("flatButtonHighContrastBorderColor"));
        theme->setProperty("controlDisabledColorChange", m_unsavedPreset.value("controlDisabledColorChange"));
        theme->setProperty("foregroundDisabledColorChange", m_unsavedPreset.value("foregroundDisabledColorChange"));
        theme->setProperty("controlHoveredColorChange", m_unsavedPreset.value("controlHoveredColorChange"));
        theme->setProperty("foregroundHoveredColorChange", m_unsavedPreset.value("foregroundHoveredColorChange"));
        theme->setProperty("controlPressedColorChange", m_unsavedPreset.value("controlPressedColorChange"));
        theme->setProperty("foregroundPressedColorChange", m_unsavedPreset.value("foregroundPressedColorChange"));
        theme->setProperty("controlCheckedColorChange", m_unsavedPreset.value("controlCheckedColorChange"));
        theme->setProperty("annotationPopupTitleColorChange", m_unsavedPreset.value("annotationPopupTitleColorChange"));
        theme->setProperty("annotationPopupContentColorChange", m_unsavedPreset.value("annotationPopupContentColorChange"));
        theme->setProperty("dockingPanelHeaderActiveColorChange", m_unsavedPreset.value("dockingPanelHeaderActiveColorChange"));

        auto trackColorSchema = CoreInterface::trackColorSchema();
        trackColorSchema->setColors({
            m_unsavedPreset.value("trackColorSchema0").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema1").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema2").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema3").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema4").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema5").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema6").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema7").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema8").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema9").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema10").value<QColor>(),
            m_unsavedPreset.value("trackColorSchema11").value<QColor>()
        });

        auto palette = RuntimeInterface::instance()->getFirstObject("org.diffscope.visualeditor.sfdefaultpalette");
        if (palette) {
            palette->setProperty("loopColor", m_unsavedPreset.value("loopColor"));
            palette->setProperty("levelMeterColor", m_unsavedPreset.value("levelMeterColor"));
            palette->setProperty("editAreaPrimaryColor", m_unsavedPreset.value("editAreaPrimaryColor"));
            palette->setProperty("editAreaSecondaryColor", m_unsavedPreset.value("editAreaSecondaryColor"));
            palette->setProperty("playheadPrimaryColor", m_unsavedPreset.value("playheadPrimaryColor"));
            palette->setProperty("playheadSecondaryColor", m_unsavedPreset.value("playheadSecondaryColor"));
            palette->setProperty("cursorIndicatorColor", m_unsavedPreset.value("cursorIndicatorColor"));
            palette->setProperty("scissorIndicatorColor", m_unsavedPreset.value("scissorIndicatorColor"));
            palette->setProperty("scalePrimaryColor", m_unsavedPreset.value("scalePrimaryColor"));
            palette->setProperty("scaleSecondaryColor", m_unsavedPreset.value("scaleSecondaryColor"));
            palette->setProperty("scaleTertiaryColor", m_unsavedPreset.value("scaleTertiaryColor"));
            palette->setProperty("levelLowColor", m_unsavedPreset.value("levelLowColor"));
            palette->setProperty("levelMediumColor", m_unsavedPreset.value("levelMediumColor"));
            palette->setProperty("levelHighColor", m_unsavedPreset.value("levelHighColor"));
            palette->setProperty("muteColor", m_unsavedPreset.value("muteColor"));
            palette->setProperty("soloColor", m_unsavedPreset.value("soloColor"));
            palette->setProperty("recordColor", m_unsavedPreset.value("recordColor"));
            palette->setProperty("routeColor", m_unsavedPreset.value("routeColor"));
            palette->setProperty("clipMuteColor", m_unsavedPreset.value("clipMuteColor"));
            palette->setProperty("whiteKeyColor", m_unsavedPreset.value("whiteKeyColor"));
            palette->setProperty("blackKeyColor", m_unsavedPreset.value("blackKeyColor"));
            palette->setProperty("whiteKeyTextColor", m_unsavedPreset.value("whiteKeyTextColor"));
            palette->setProperty("blackKeyTextColor", m_unsavedPreset.value("blackKeyTextColor"));
            palette->setProperty("itemSelectedColorChange", m_unsavedPreset.value("itemSelectedColorChange"));
            palette->setProperty("clipSelectedColorChange", m_unsavedPreset.value("clipSelectedColorChange"));
            palette->setProperty("clipThumbnailColorChange", m_unsavedPreset.value("clipThumbnailColorChange"));
            palette->setProperty("noteSelectedColorChange", m_unsavedPreset.value("noteSelectedColorChange"));
        }
    }
    static const char settingCategoryC[] = "Core::Internal::ColorSchemeCollection";
    void ColorSchemeCollection::load() {
        auto settings = RuntimeInterface::settings();
        auto data = settings->value(settingCategoryC).toMap();
        if (!(data.contains("presets") && data.contains("unsavedPreset") && data.contains("currentIndex") && data.contains("showUnsavedPreset"))) {
            return;
        }
        QList<QPair<QString, QVariantHash>> presets;
        std::ranges::transform(data.value("presets").toList(), std::back_inserter(presets), [](const QVariant &v) {
            auto p = v.value<QVariantPair>();
            return qMakePair(p.first.toString(), deserializePreset(p.second.toByteArray()));
        });
        m_presets = presets;
        m_unsavedPreset = deserializePreset(data.value("unsavedPreset").toByteArray());
        m_currentIndex = data.value("currentIndex").toInt();
        m_showUnsavedPreset = data.value("showUnsavedPreset").toBool();
        if (!m_showUnsavedPreset) {
            const auto &[_, preset] = m_currentIndex < MAX_INTERNAL_PRESETS ? m_internalPresets.value(m_currentIndex, m_internalPresets.first()) : m_presets.value(m_currentIndex - MAX_INTERNAL_PRESETS);
            m_unsavedPreset = preset;
        }
        emit allPresetsChanged();
        emit currentIndexChanged();
    }
    void ColorSchemeCollection::save() const {
        auto settings = RuntimeInterface::settings();
        QVariantList presetsVariantList;
        std::ranges::transform(m_presets, std::back_inserter(presetsVariantList), [](const auto &p) {
            return QVariant::fromValue(QVariantPair(p.first, serializePreset(p.second)));
        });
        auto data = QVariantMap{
            {"presets", presetsVariantList},
            {"unsavedPreset", serializePreset(m_unsavedPreset)},
            {"currentIndex", m_currentIndex},
            {"showUnsavedPreset", m_showUnsavedPreset},
        };
        settings->setValue(settingCategoryC, data);
    }

    QList<QPair<QString, QVariantHash>> ColorSchemeCollection::internalPresets() {
        return m_internalPresets;
    }
}
