// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H
#define DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H

#include <QVariantList>

#include <CoreApi/isettingpage.h>

#include <coreplugin/internal/ActionConfiguration.h>

class QAbstractItemModel;
class QWindow;

namespace Core::Internal {
    class ActionCatalogProxyModel;

    class KeyMapPage : public ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)
        Q_PROPERTY(QVariantList presets READ presets NOTIFY stateChanged)
        Q_PROPERTY(QString currentPresetId READ currentPresetId WRITE setCurrentPresetId NOTIFY stateChanged)
        Q_PROPERTY(bool canEdit READ canEdit NOTIFY stateChanged)
        Q_PROPERTY(bool canRestoreAction READ canRestoreAction NOTIFY stateChanged)
        Q_PROPERTY(QString currentShortcut READ currentShortcut NOTIFY stateChanged)
        Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
        Q_PROPERTY(QString shortcutFilterText READ shortcutFilterText NOTIFY stateChanged)
    public:
        explicit KeyMapPage(QObject *parent = nullptr);
        ~KeyMapPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QAbstractItemModel *model() const;
        QVariantList presets() const;
        QString currentPresetId() const;
        void setCurrentPresetId(const QString &id);
        bool canEdit() const;
        bool canRestoreAction() const;
        QString currentShortcut() const;
        QString errorText() const;
        QString shortcutFilterText() const;

        Q_INVOKABLE void selectAction(const QString &id);
        Q_INVOKABLE bool saveAs(const QString &name);
        Q_INVOKABLE bool renamePreset(const QString &name);
        Q_INVOKABLE void deletePreset();
        Q_INVOKABLE void editShortcut(QWindow *parentWindow);
        Q_INVOKABLE void clearShortcut();
        Q_INVOKABLE void restoreAction();
        Q_INVOKABLE void restoreAll();
        Q_INVOKABLE void findShortcut(QWindow *parentWindow);
        Q_INVOKABLE void clearShortcutFilter();

    Q_SIGNALS:
        void stateChanged();

    private:
        KeymapOverrides currentOverrides() const;
        QKeySequence shortcut(const QString &id) const;
        QString inputError(const QKeySequence &key) const;
        QString bindingError(const QString &id, const QKeySequence &key) const;
        QKeySequence recordShortcut(const QKeySequence &initial, bool finding, QWindow *parentWindow);
        bool validateName(const QString &name, const QString &exceptId = {});
        KeymapPreset &editablePreset();
        void setShortcut(const QKeySequence &key);
        void changed();
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        ActionCatalogProxyModel *m_model{};
        QList<KeymapPreset> m_presets;
        QString m_currentPresetId;
        QString m_selectedActionId;
        QString m_errorText;
        QKeySequence m_shortcutFilter;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H
