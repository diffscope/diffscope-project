// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ActionConfiguration.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcActionConfiguration, "diffscope.core.actionconfiguration")

    static QAK::ActionFamily::ShortcutsFamily shortcutsFamily(const KeymapOverrides &overrides) {
        QAK::ActionFamily::ShortcutsFamily family;
        auto registry = CoreInterface::actionRegistry();
        for (const auto &id : registry->actionIds()) {
            const auto info = registry->actionInfo(id);
            if (info && info->isCommand()) {
                const auto shortcut = overrides.value(id, info->shortcuts().value(0));
                family.insert(id, shortcut.isEmpty() ? QList<QKeySequence>() : QList<QKeySequence>{shortcut});
            }
        }
        return family;
    }

    ActionConfiguration::ActionConfiguration(QObject *parent) : QObject(parent) {
    }

    ActionConfiguration *ActionConfiguration::get() {
        return RuntimeInterface::instance()->getFirstObject<ActionConfiguration>();
    }

    void ActionConfiguration::load() {
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        const auto layoutData = QJsonDocument::fromJson(settings->value("layoutChanges").toByteArray());
        const auto keymapData = QJsonDocument::fromJson(settings->value("keymapPresets").toByteArray());
        m_currentKeymapId = settings->value("currentKeymapId").toString();
        settings->endGroup();

        QVector<QAK::ActionLayoutChange> changes;
        for (const auto &value : layoutData.array()) {
            if (auto change = QAK::ActionLayoutChange::fromJsonObject(value.toObject())) {
                changes.append(*change);
            } else {
                qCWarning(lcActionConfiguration) << "A layout change is invalid.";
            }
        }
        CoreInterface::actionRegistry()->setLayoutChanges(changes);

        m_keymapPresets.clear();
        KeymapOverrides selected;
        bool found = m_currentKeymapId.isEmpty();
        for (const auto &value : keymapData.array()) {
            const auto object = value.toObject();
            const auto id = object.value("id").toString();
            const auto name = object.value("name").toString().trimmed();
            if (id.isEmpty() || name.isEmpty()) {
                continue;
            }
            KeymapPreset preset{id, name, {}};
            const auto overrides = object.value("overrides").toObject();
            for (auto it = overrides.begin(); it != overrides.end(); ++it) {
                if (it.value().isString()) {
                    preset.overrides.insert(it.key(), QKeySequence::fromString(it.value().toString(), QKeySequence::PortableText));
                }
            }
            m_keymapPresets.append(preset);
            if (id == m_currentKeymapId) {
                selected = preset.overrides;
                found = true;
            }
        }
        if (!found) {
            m_currentKeymapId.clear();
        }
        CoreInterface::actionRegistry()->setShortcutsFamily(shortcutsFamily(selected));
    }

    bool ActionConfiguration::saveLayouts(const QAK::ActionLayouts &layouts) {
        auto registry = CoreInterface::actionRegistry();
        const auto changes = registry->computeLayoutChanges(layouts);
        QAK::ActionRegistry replay;
        replay.setExtensions(registry->extensions());
        replay.setLayoutChanges(changes);
        if (replay.layouts().adjacencyMap() != layouts.adjacencyMap()) {
            qCWarning(lcActionConfiguration) << "Layout changes do not reproduce the edited layouts.";
            return false;
        }
        QJsonArray array;
        for (const auto &change : changes) {
            array.append(change.toJsonObject());
        }
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        const auto previous = settings->value("layoutChanges");
        settings->setValue("layoutChanges", QJsonDocument(array).toJson(QJsonDocument::Compact));
        settings->endGroup();
        settings->sync();
        if (settings->status() != QSettings::NoError) {
            settings->beginGroup(staticMetaObject.className());
            if (previous.isValid()) {
                settings->setValue("layoutChanges", previous);
            } else {
                settings->remove("layoutChanges");
            }
            settings->endGroup();
            qCWarning(lcActionConfiguration) << "Saving the layout changes failed.";
            return false;
        }
        registry->setLayoutChanges(changes);
        registry->updateContext(QAK::AE_Layouts);
        return true;
    }

    bool ActionConfiguration::saveKeymaps(const QList<KeymapPreset> &presets, const QString &currentId) {
        QJsonArray array;
        KeymapOverrides selected;
        for (const auto &preset : presets) {
            QJsonObject overrides;
            for (auto it = preset.overrides.begin(); it != preset.overrides.end(); ++it) {
                overrides.insert(it.key(), it.value().toString(QKeySequence::PortableText));
            }
            array.append(QJsonObject{
                {"id", preset.id},
                {"name", preset.name},
                {"overrides", overrides},
            });
            if (preset.id == currentId) {
                selected = preset.overrides;
            }
        }
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        const auto previousPresets = settings->value("keymapPresets");
        const auto previousId = settings->value("currentKeymapId");
        settings->setValue("keymapPresets", QJsonDocument(array).toJson(QJsonDocument::Compact));
        settings->setValue("currentKeymapId", currentId);
        settings->endGroup();
        settings->sync();
        if (settings->status() != QSettings::NoError) {
            settings->beginGroup(staticMetaObject.className());
            if (previousPresets.isValid()) {
                settings->setValue("keymapPresets", previousPresets);
            } else {
                settings->remove("keymapPresets");
            }
            if (previousId.isValid()) {
                settings->setValue("currentKeymapId", previousId);
            } else {
                settings->remove("currentKeymapId");
            }
            settings->endGroup();
            qCWarning(lcActionConfiguration) << "Saving the keyboard mapping presets failed.";
            return false;
        }
        m_keymapPresets = presets;
        m_currentKeymapId = currentId;
        auto registry = CoreInterface::actionRegistry();
        registry->setShortcutsFamily(shortcutsFamily(selected));
        registry->updateContext(QAK::AE_Keymap);
        return true;
    }

    QList<KeymapPreset> ActionConfiguration::keymapPresets() const {
        return m_keymapPresets;
    }

    QString ActionConfiguration::currentKeymapId() const {
        return m_currentKeymapId;
    }
}

#include "moc_ActionConfiguration.cpp"
