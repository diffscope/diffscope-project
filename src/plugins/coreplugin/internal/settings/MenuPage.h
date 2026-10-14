// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_MENUPAGE_H
#define DIFFSCOPE_COREPLUGIN_MENUPAGE_H

#include <QModelIndex>
#include <QVariantList>

#include <CoreApi/isettingpage.h>

namespace QAK {
    class ActionLayouts;
    class ActionLayoutsModel;
}

namespace Core::Internal {
    class ActionCatalogProxyModel;
    class LayoutFilterProxyModel;

    class MenuPage : public ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *catalogModel READ catalogModel CONSTANT)
        Q_PROPERTY(QModelIndex currentIndex READ currentIndex NOTIFY stateChanged)
        Q_PROPERTY(bool canAdd READ canAdd NOTIFY stateChanged)
        Q_PROPERTY(bool canRemove READ canRemove NOTIFY stateChanged)
        Q_PROPERTY(bool canMoveUp READ canMoveUp NOTIFY stateChanged)
        Q_PROPERTY(bool canMoveDown READ canMoveDown NOTIFY stateChanged)
        Q_PROPERTY(bool canChangeForm READ canChangeForm NOTIFY stateChanged)
        Q_PROPERTY(bool canRestoreCurrent READ canRestoreCurrent NOTIFY stateChanged)
        Q_PROPERTY(int currentForm READ currentForm NOTIFY stateChanged)
        Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    public:
        explicit MenuPage(QObject *parent = nullptr);
        ~MenuPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QAbstractItemModel *catalogModel() const;
        QModelIndex currentIndex() const;
        bool canAdd() const;
        bool canRemove() const;
        bool canMoveUp() const;
        bool canMoveDown() const;
        bool canChangeForm() const;
        bool canRestoreCurrent() const;
        int currentForm() const;
        QString errorText() const;
        Q_INVOKABLE void selectIndex(const QModelIndex &index);
        Q_INVOKABLE void setSearchText(const QString &text);
        Q_INVOKABLE QVariantList pathForIndex(const QModelIndex &index) const;
        Q_INVOKABLE QVariantList expansionKey(const QModelIndex &index) const;
        Q_INVOKABLE QModelIndex indexForExpansionKey(const QVariantList &key) const;
        Q_INVOKABLE QString typeText(const QString &id, int type) const;
        Q_INVOKABLE bool addEntry(const QString &id, int type);
        Q_INVOKABLE bool removeEntry();
        Q_INVOKABLE bool moveCurrent(int direction);
        Q_INVOKABLE bool dropEntry(const QVariantList &path, const QModelIndex &target, int placement);
        Q_INVOKABLE bool changeForm(int type);
        Q_INVOKABLE bool restoreCurrent();
        Q_INVOKABLE void restoreAll();

    Q_SIGNALS:
        void stateChanged();

    private:
        QModelIndex sourceIndex() const;
        QModelIndex indexForPath(const QVariantList &path) const;
        bool moveEntry(const QVariantList &path, QVariantList parentPath, int row);
        bool prepareRestore(QAK::ActionLayouts &layouts) const;
        void changed();
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        QAK::ActionLayoutsModel *m_actionLayoutsModel{};
        ActionCatalogProxyModel *m_catalogModel{};
        LayoutFilterProxyModel *m_proxy{};
        QVariantList m_currentPath;
        QString m_errorText;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_MENUPAGE_H
