#ifndef DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H
#define DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H

#include <QObject>
#include <QStringList>
#include <QList>

class QQuickItem;

namespace VisualEditor::Internal {

    class AdditionalTrackLoader : public QObject {
        Q_OBJECT

        Q_PROPERTY(QStringList components READ components CONSTANT)
        Q_PROPERTY(QStringList loadedComponents READ loadedComponents NOTIFY loadedComponentsChanged)
        Q_PROPERTY(QList<QQuickItem*> loadedItems READ loadedItems NOTIFY loadedItemsChanged)

    public:
        explicit AdditionalTrackLoader(const QString &categoryId, QObject *parent = nullptr);
        ~AdditionalTrackLoader() override;

        void setContextObject(QObject *contextObject);

        QStringList components() const;
        QStringList loadedComponents() const;
        QList<QQuickItem*> loadedItems() const;

        Q_INVOKABLE void moveUp(const QString &id);
        Q_INVOKABLE void moveDown(const QString &id);
        Q_INVOKABLE void loadItem(const QString &id);
        Q_INVOKABLE void removeItem(const QString &id);

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
        QObject *m_contextObject{};
        QList<QPair<QString, QQuickItem*>> m_loaded;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_ADDITIONALTRACKLOADER_H
