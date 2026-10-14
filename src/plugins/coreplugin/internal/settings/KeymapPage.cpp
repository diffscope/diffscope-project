// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "KeymapPage.h"

#include <algorithm>

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QPoint>
#include <QPushButton>
#include <QQmlComponent>
#include <QStringList>
#include <QUuid>
#include <QVBoxLayout>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ActionCatalogProxyModel.h>

namespace Core::Internal {

    static bool overlaps(const QKeySequence &a, const QKeySequence &b) {
        return a.matches(b) != QKeySequence::NoMatch || b.matches(a) != QKeySequence::NoMatch;
    }

    KeyMapPage::KeyMapPage(QObject *parent) : ISettingPage("org.diffscope.core.Keymap", parent) {
        setTitle(tr("Keymap"));
        setDescription(tr("Configure shortcuts of actions"));
        m_model = new ActionCatalogProxyModel(this);
        m_model->setCommandsOnly(true);
    }

    KeyMapPage::~KeyMapPage() {
        delete m_widget;
    }

    bool KeyMapPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString KeyMapPage::sortKeyword() const {
        return QStringLiteral("Keymap");
    }

    QObject *KeyMapPage::widget() {
        if (!m_widget) {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "KeymapPage");
            m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
            if (!m_widget) {
                qFatal() << component.errorString();
            }
            m_widget->setParent(this);
        }
        return m_widget;
    }

    void KeyMapPage::beginSetting() {
        widget();
        m_presets = ActionConfiguration::get()->keymapPresets();
        m_currentPresetId = ActionConfiguration::get()->currentKeymapId();
        m_selectedActionId.clear();
        m_errorText.clear();
        m_shortcutFilter = {};
        m_model->refresh();
        m_model->setShortcutFilter({});
        m_model->setShortcutOverrides(currentOverrides());
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
        Q_EMIT stateChanged();
    }

    bool KeyMapPage::accept() {
        for (const auto &id : CoreInterface::actionRegistry()->actionIds()) {
            const auto info = CoreInterface::actionRegistry()->actionInfo(id);
            if (info && info->isCommand()) {
                m_errorText = bindingError(id, shortcut(id));
                if (!m_errorText.isEmpty()) {
                    Q_EMIT stateChanged();
                    return false;
                }
            }
        }
        if (!ActionConfiguration::get()->saveKeymaps(m_presets, m_currentPresetId)) {
            m_errorText = tr("The keyboard mapping settings could not be saved.");
            Q_EMIT stateChanged();
            return false;
        }
        return ISettingPage::accept();
    }

    void KeyMapPage::endSetting() {
        if (m_widget) {
            m_widget->setProperty("started", false);
        }
        ISettingPage::endSetting();
    }

    QAbstractItemModel *KeyMapPage::model() const {
        return m_model;
    }

    QVariantList KeyMapPage::presets() const {
        QVariantList result{QVariantMap{{"id", QString()}, {"name", tr("Default")}}};
        for (const auto &preset : m_presets) {
            result.append(QVariantMap{{"id", preset.id}, {"name", preset.name}});
        }
        return result;
    }

    QString KeyMapPage::currentPresetId() const {
        return m_currentPresetId;
    }

    void KeyMapPage::setCurrentPresetId(const QString &id) {
        if (id == m_currentPresetId) {
            return;
        }
        if (!id.isEmpty() && std::none_of(m_presets.begin(), m_presets.end(), [&](const auto &preset) { return preset.id == id; })) {
            return;
        }
        m_currentPresetId = id;
        changed();
    }

    KeymapOverrides KeyMapPage::currentOverrides() const {
        for (const auto &preset : m_presets) {
            if (preset.id == m_currentPresetId) {
                return preset.overrides;
            }
        }
        return {};
    }

    QKeySequence KeyMapPage::shortcut(const QString &id) const {
        const auto overrides = currentOverrides();
        if (const auto it = overrides.constFind(id); it != overrides.cend()) {
            return it.value();
        }
        const auto info = CoreInterface::actionRegistry()->actionInfo(id);
        return info ? info->shortcuts().value(0) : QKeySequence();
    }

    bool KeyMapPage::canEdit() const {
        const auto info = CoreInterface::actionRegistry()->actionInfo(m_selectedActionId);
        return info && info->isCommand();
    }

    bool KeyMapPage::canRestoreAction() const {
        return canEdit() && currentOverrides().contains(m_selectedActionId);
    }

    QString KeyMapPage::currentShortcut() const {
        return shortcut(m_selectedActionId).toString(QKeySequence::NativeText);
    }

    QString KeyMapPage::errorText() const {
        return m_errorText;
    }

    QString KeyMapPage::shortcutFilterText() const {
        return m_shortcutFilter.toString(QKeySequence::NativeText);
    }

    void KeyMapPage::selectAction(const QString &id) {
        if (m_selectedActionId != id) {
            m_selectedActionId = id;
            m_errorText.clear();
            Q_EMIT stateChanged();
        }
    }

    bool KeyMapPage::validateName(const QString &name, const QString &exceptId) {
        if (name.trimmed().isEmpty()) {
            m_errorText = tr("Enter a preset name.");
        } else if (name.compare(tr("Default"), Qt::CaseInsensitive) == 0 ||
                   std::any_of(m_presets.begin(), m_presets.end(), [&](const auto &preset) {
                       return preset.id != exceptId && preset.name.compare(name, Qt::CaseInsensitive) == 0;
                   })) {
            m_errorText = tr("A preset with this name already exists.");
        } else {
            m_errorText.clear();
            return true;
        }
        Q_EMIT stateChanged();
        return false;
    }

    bool KeyMapPage::saveAs(const QString &name) {
        const auto trimmed = name.trimmed();
        if (!validateName(trimmed)) {
            return false;
        }
        const auto overrides = currentOverrides();
        const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_presets.append({id, trimmed, overrides});
        m_currentPresetId = id;
        changed();
        return true;
    }

    bool KeyMapPage::renamePreset(const QString &name) {
        const auto trimmed = name.trimmed();
        if (m_currentPresetId.isEmpty() || !validateName(trimmed, m_currentPresetId)) {
            return false;
        }
        for (auto &preset : m_presets) {
            if (preset.id == m_currentPresetId) {
                if (preset.name != trimmed) {
                    preset.name = trimmed;
                    changed();
                } else {
                    Q_EMIT stateChanged();
                }
                return true;
            }
        }
        return false;
    }

    void KeyMapPage::deletePreset() {
        if (m_currentPresetId.isEmpty()) {
            return;
        }
        m_presets.removeIf([&](const auto &preset) { return preset.id == m_currentPresetId; });
        m_currentPresetId.clear();
        changed();
    }

    KeymapPreset &KeyMapPage::editablePreset() {
        for (auto &preset : m_presets) {
            if (preset.id == m_currentPresetId) {
                return preset;
            }
        }
        const auto base = tr("Default (copy)");
        auto name = base;
        int suffix = 2;
        while (std::any_of(m_presets.begin(), m_presets.end(), [&](const auto &preset) {
            return preset.name.compare(name, Qt::CaseInsensitive) == 0;
        })) {
            name = tr("%1 (%L2)").arg(base).arg(suffix++);
        }
        m_currentPresetId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_presets.append({m_currentPresetId, name, {}});
        return m_presets.last();
    }

    QString KeyMapPage::inputError(const QKeySequence &key) const {
        if (key.isEmpty()) {
            return tr("Enter a shortcut.");
        }
        for (int part = 0; part < key.count(); ++part) {
            const auto code = key[part].key();
            if (code == Qt::Key_unknown || code == Qt::Key_Control || code == Qt::Key_Shift || code == Qt::Key_Alt || code == Qt::Key_Meta || code == Qt::Key_AltGr || code == 0) {
                return tr("A shortcut must include a key other than a modifier.");
            }
        }
        return {};
    }

    QString KeyMapPage::bindingError(const QString &id, const QKeySequence &key) const {
        if (key.isEmpty()) {
            return {};
        }
        if (const auto error = inputError(key); !error.isEmpty()) {
            return error;
        }
        QStringList conflicts;
        auto registry = CoreInterface::actionRegistry();
        for (const auto &otherId : registry->actionIds()) {
            if (otherId == id) {
                continue;
            }
            const auto info = registry->actionInfo(otherId);
            if (!info || !info->isCommand()) {
                continue;
            }
            const auto otherKey = shortcut(otherId);
            if (!otherKey.isEmpty() && overlaps(key, otherKey)) {
                const auto category = info->category().withoutMnemonic();
                const auto name = info->text().withoutMnemonic();
                conflicts.append(category.isEmpty() ? name : tr("%1: %2").arg(category, name));
            }
        }
        return conflicts.isEmpty() ? QString() : tr("Already assigned to:\n%1").arg(conflicts.join(u'\n'));
    }

    QKeySequence KeyMapPage::recordShortcut(const QKeySequence &initial, bool finding, QWindow *parentWindow) {
        QDialog dialog;
        dialog.setWindowTitle(finding ? tr("Find Shortcut") : tr("Edit Shortcut"));
        dialog.setWindowModality(Qt::WindowModal);
        dialog.winId();
        dialog.windowHandle()->setTransientParent(parentWindow);
        auto layout = new QVBoxLayout(&dialog);
        auto recorder = new QKeySequenceEdit(initial, &dialog);
        recorder->setMaximumSequenceLength(4);
        recorder->setClearButtonEnabled(true);
        layout->addWidget(recorder);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        const auto validate = [&] {
            buttons->button(QDialogButtonBox::Ok)->setEnabled(inputError(recorder->keySequence()).isEmpty());
        };
        connect(recorder, &QKeySequenceEdit::keySequenceChanged, &dialog, validate);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        validate();
        recorder->setFocus();
        dialog.adjustSize();
        if (parentWindow) {
            dialog.move(parentWindow->geometry().center() - QPoint(dialog.width() / 2, dialog.height() / 2));
        }
        return dialog.exec() == QDialog::Accepted ? recorder->keySequence() : QKeySequence();
    }

    void KeyMapPage::setShortcut(const QKeySequence &key) {
        if (!canEdit() || key == shortcut(m_selectedActionId)) {
            return;
        }
        m_errorText = bindingError(m_selectedActionId, key);
        if (!m_errorText.isEmpty()) {
            Q_EMIT stateChanged();
            return;
        }
        const auto info = CoreInterface::actionRegistry()->actionInfo(m_selectedActionId);
        auto &overrides = editablePreset().overrides;
        if (key == info->shortcuts().value(0)) {
            overrides.remove(m_selectedActionId);
        } else {
            overrides.insert(m_selectedActionId, key);
        }
        changed();
    }

    void KeyMapPage::editShortcut(QWindow *parentWindow) {
        if (!canEdit()) {
            return;
        }
        const auto key = recordShortcut(shortcut(m_selectedActionId), false, parentWindow);
        if (!key.isEmpty()) {
            setShortcut(key);
        }
    }

    void KeyMapPage::clearShortcut() {
        setShortcut({});
    }

    void KeyMapPage::restoreAction() {
        if (!canRestoreAction()) {
            return;
        }
        const auto info = CoreInterface::actionRegistry()->actionInfo(m_selectedActionId);
        m_errorText = bindingError(m_selectedActionId, info->shortcuts().value(0));
        if (!m_errorText.isEmpty()) {
            Q_EMIT stateChanged();
            return;
        }
        editablePreset().overrides.remove(m_selectedActionId);
        changed();
    }

    void KeyMapPage::restoreAll() {
        if (m_currentPresetId.isEmpty() || currentOverrides().isEmpty()) {
            return;
        }
        auto &preset = editablePreset();
        const auto snapshot = preset.overrides;
        preset.overrides.clear();
        for (const auto &id : CoreInterface::actionRegistry()->actionIds()) {
            const auto info = CoreInterface::actionRegistry()->actionInfo(id);
            if (info && info->isCommand()) {
                m_errorText = bindingError(id, info->shortcuts().value(0));
                if (!m_errorText.isEmpty()) {
                    preset.overrides = snapshot;
                    Q_EMIT stateChanged();
                    return;
                }
            }
        }
        changed();
    }

    void KeyMapPage::findShortcut(QWindow *parentWindow) {
        const auto key = recordShortcut(m_shortcutFilter, true, parentWindow);
        if (!key.isEmpty()) {
            m_shortcutFilter = key;
            m_model->setShortcutFilter(key);
            Q_EMIT stateChanged();
        }
    }

    void KeyMapPage::clearShortcutFilter() {
        m_shortcutFilter = {};
        m_model->setShortcutFilter({});
        Q_EMIT stateChanged();
    }

    void KeyMapPage::changed() {
        m_errorText.clear();
        m_model->setShortcutOverrides(currentOverrides());
        markDirty();
        Q_EMIT stateChanged();
    }

    bool KeyMapPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", Q_RETURN_ARG(bool, result), word);
        return result;
    }
}

#include "moc_KeymapPage.cpp"
