// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H
#define DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QStringList>
#include <QUrl>

class QQuickItem;

Q_MOC_INCLUDE(<QQuickItem>)

namespace VisualEditor::Internal {

    class AdditionalTrackLoader : public QObject {
        Q_OBJECT

        Q_PROPERTY(QStringList components READ components CONSTANT)
        Q_PROPERTY(QStringList loadedComponents READ loadedComponents NOTIFY loadedComponentsChanged)
        Q_PROPERTY(QList<QQuickItem *> loadedItems READ loadedItems NOTIFY loadedItemsChanged)

    public:
        explicit AdditionalTrackLoader(const QString &categoryId, QObject *parent = nullptr);
        ~AdditionalTrackLoader() override;

        void setContextObject(QObject *contextObject);

        QStringList components() const;
        QStringList loadedComponents() const;
        QList<QQuickItem *> loadedItems() const;

        Q_INVOKABLE void moveUp(const QString &id);
        Q_INVOKABLE void moveDown(const QString &id);
        Q_INVOKABLE void loadItem(const QString &id);
        Q_INVOKABLE void removeItem(const QString &id);
        Q_INVOKABLE bool prepareItems(const QStringList &ids);
        Q_INVOKABLE bool commitPreparedItems();
        Q_INVOKABLE void finishPreparedItems();
        Q_INVOKABLE void abortPreparedItems();

        Q_INVOKABLE static QString componentName(const QString &id);
        Q_INVOKABLE static QUrl componentIcon(const QString &id);

    Q_SIGNALS:
        void loadedComponentsChanged();
        void loadedItemsChanged();

    private:
        void updateComponentIdList();
        QQuickItem *createItem(const QString &id);

    private:
        QStringList m_components;
        QString m_categoryId;
        QPointer<QObject> m_contextObject;
        QList<QPair<QString, QPointer<QQuickItem>>> m_loaded;
        QList<QPair<QString, QPointer<QQuickItem>>> m_originalLoaded;
        QList<QPair<QString, QPointer<QQuickItem>>> m_preparedLoaded;
        QList<QPointer<QQuickItem>> m_createdItems;
        bool m_prepared = false;
        bool m_committed = false;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H
