// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AdditionalTrackLoader.h"

#include <algorithm>
#include <iterator>
#include <memory>

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSet>
#include <QUrl>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAdditionalTrackLoader, "diffscope.visualeditor.additionaltrackloader")

    AdditionalTrackLoader::AdditionalTrackLoader(const QString &categoryId, QObject *parent)
        : QObject(parent), m_categoryId(categoryId) {
        updateComponentIdList();
    }

    AdditionalTrackLoader::~AdditionalTrackLoader() = default;

    void AdditionalTrackLoader::setContextObject(QObject *contextObject) {
        m_contextObject = contextObject;
    }

    QStringList AdditionalTrackLoader::components() const {
        return m_components;
    }

    QStringList AdditionalTrackLoader::loadedComponents() const {
        QStringList list;
        list.reserve(m_loaded.size());
        for (const auto &[id, item] : m_loaded)
            if (item)
                list.append(id);
        return list;
    }

    QList<QQuickItem *> AdditionalTrackLoader::loadedItems() const {
        QList<QQuickItem *> list;
        list.reserve(m_loaded.size());
        for (const auto &[id, item] : m_loaded)
            if (item)
                list.append(item.data());
        return list;
    }

    void AdditionalTrackLoader::moveUp(const QString &id) {
        if (m_prepared)
            return;
        auto it = std::ranges::find_if(m_loaded, [&id](const auto &p) {
            return p.first == id;
        });
        if (it == m_loaded.end())
            return;
        int i = std::distance(m_loaded.begin(), it);
        if (i == 0)
            return;
        m_loaded.swapItemsAt(i, i - 1);
        Q_EMIT loadedComponentsChanged();
        Q_EMIT loadedItemsChanged();
    }

    void AdditionalTrackLoader::moveDown(const QString &id) {
        if (m_prepared)
            return;
        auto it = std::ranges::find_if(m_loaded, [&id](const auto &p) {
            return p.first == id;
        });
        if (it == m_loaded.end())
            return;
        int i = std::distance(m_loaded.begin(), it);
        if (i == m_loaded.size() - 1)
            return;
        m_loaded.swapItemsAt(i, i + 1);
        Q_EMIT loadedComponentsChanged();
        Q_EMIT loadedItemsChanged();
    }

    void AdditionalTrackLoader::loadItem(const QString &id) {
        if (m_prepared)
            return;
        if (std::ranges::any_of(m_loaded, [&id](const auto &p) { return p.first == id; }))
            return;

        QQuickItem *item = createItem(id);
        if (!item)
            return;

        m_loaded.emplaceBack(id, item);
        Q_EMIT loadedComponentsChanged();
        Q_EMIT loadedItemsChanged();
    }

    void AdditionalTrackLoader::removeItem(const QString &id) {
        if (m_prepared)
            return;
        auto it = std::ranges::find_if(m_loaded, [&id](const auto &p) {
            return p.first == id;
        });
        if (it == m_loaded.end())
            return;
        auto item = it->second;
        m_loaded.erase(it);
        if (item)
            item->deleteLater();
        Q_EMIT loadedComponentsChanged();
        Q_EMIT loadedItemsChanged();
    }

    QString AdditionalTrackLoader::componentName(const QString &id) {
        auto info = Core::CoreInterface::actionRegistry()->actionInfo(id);
        return info ? info->text().toString() : QString();
    }

    QUrl AdditionalTrackLoader::componentIcon(const QString &id) {
        auto icon = Core::CoreInterface::actionRegistry()->actionIcon("", id, id);
        return icon ? icon->url() : QUrl();
    }

    void AdditionalTrackLoader::updateComponentIdList() {
        m_components.clear();
        auto actions = Core::CoreInterface::actionRegistry()->catalog().children(m_categoryId);
        m_components = actions;
    }

    QQuickItem *AdditionalTrackLoader::createItem(const QString &id) {
        if (!m_contextObject)
            return nullptr;
        auto windowInterface = m_contextObject->property("windowHandle").value<Core::ProjectWindowInterface *>();
        if (!windowInterface)
            return nullptr;
        auto component = windowInterface->actionContext()->action(id);
        if (!component || !component->isReady()) {
            qCWarning(lcAdditionalTrackLoader) << "Component" << id << "not found";
            return nullptr;
        }
        std::unique_ptr<QObject> object(component->createWithInitialProperties({
                                                                                   {"contextObject", QVariant::fromValue(m_contextObject.data())},
                                                                               },
                                                                               component->creationContext()));
        if (!object) {
            qCWarning(lcAdditionalTrackLoader) << "Failed to create component" << id << component->errorString();
            return nullptr;
        }
        windowInterface->actionContext()->attachActionInfo(id, object.get());
        if (!qobject_cast<QQuickItem *>(object.get())) {
            qCWarning(lcAdditionalTrackLoader) << "Component" << id << "is not a QuickItem";
            return nullptr;
        }
        auto item = qobject_cast<QQuickItem *>(object.release());
        item->setParent(this);
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        connect(item, &QObject::destroyed, this, [this, id] {
            if (m_loaded.removeIf([&](const auto &entry) { return entry.first == id && entry.second.isNull(); })) {
                Q_EMIT loadedComponentsChanged();
                Q_EMIT loadedItemsChanged();
            }
        });
        return item;
    }

    bool AdditionalTrackLoader::prepareItems(const QStringList &ids) {
        if (m_prepared)
            return false;

        QSet<QString> unique;
        for (const auto &id : ids) {
            if (!m_components.contains(id) || unique.contains(id))
                return false;
            unique.insert(id);
        }

        m_prepared = true;
        m_originalLoaded = m_loaded;
        for (const auto &id : ids) {
            auto it = std::ranges::find_if(m_loaded, [&](const auto &entry) { return entry.first == id && entry.second; });
            bool existing = it != m_loaded.end();
            QPointer<QQuickItem> item = existing ? it->second.data() : createItem(id);
            if (!item) {
                abortPreparedItems();
                return false;
            }
            if (!existing)
                m_createdItems.append(item);
            m_preparedLoaded.append({id, item});
        }
        return true;
    }

    bool AdditionalTrackLoader::commitPreparedItems() {
        if (!m_prepared || m_committed)
            return false;
        if (std::ranges::any_of(m_preparedLoaded, [](const auto &entry) { return entry.second.isNull(); }))
            return false;

        bool changed = m_loaded != m_preparedLoaded;
        m_loaded = m_preparedLoaded;
        m_committed = true;
        if (changed) {
            Q_EMIT loadedComponentsChanged();
            Q_EMIT loadedItemsChanged();
        }
        return true;
    }

    void AdditionalTrackLoader::finishPreparedItems() {
        if (!m_prepared || !m_committed)
            return;

        for (const auto &entry : m_originalLoaded) {
            if (!entry.second)
                continue;
            if (std::ranges::none_of(m_loaded, [&](const auto &current) { return current.second == entry.second; }))
                entry.second->deleteLater();
        }

        m_originalLoaded.clear();
        m_preparedLoaded.clear();
        m_createdItems.clear();
        m_prepared = false;
        m_committed = false;
    }

    void AdditionalTrackLoader::abortPreparedItems() {
        if (!m_prepared)
            return;

        bool changed = m_committed && m_loaded != m_originalLoaded;
        if (m_committed)
            m_loaded = m_originalLoaded;
        for (auto item : m_createdItems)
            if (item)
                item->deleteLater();

        m_originalLoaded.clear();
        m_preparedLoaded.clear();
        m_createdItems.clear();
        m_prepared = false;
        m_committed = false;
        if (changed) {
            Q_EMIT loadedComponentsChanged();
            Q_EMIT loadedItemsChanged();
        }
    }

}

#include "moc_AdditionalTrackLoader.cpp"
