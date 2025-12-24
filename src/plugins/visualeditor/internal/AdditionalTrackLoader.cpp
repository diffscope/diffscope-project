#include "AdditionalTrackLoader.h"

#include <algorithm>
#include <iterator>

#include <QQuickItem>
#include <QLoggingCategory>
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
        std::ranges::transform(m_loaded, std::back_inserter(list), [](const auto &p) {
            return p.first;
        });
        return list;
    }

    QList<QQuickItem*> AdditionalTrackLoader::loadedItems() const {
        QList<QQuickItem*> list;
        list.reserve(m_loaded.size());
        std::ranges::transform(m_loaded, std::back_inserter(list), [](const auto &p) {
            return p.second;
        });
        return list;
    }

    void AdditionalTrackLoader::moveUp(const QString &id) {
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
        auto it = std::ranges::find_if(m_loaded, [&id](const auto &p) {
            return p.first == id;
        });
        if (it == m_loaded.end())
            return;
        auto item = it->second;
        m_loaded.erase(it);
        item->deleteLater();
        Q_EMIT loadedComponentsChanged();
        Q_EMIT loadedItemsChanged();
    }

    QString AdditionalTrackLoader::componentName(const QString &id) {
        auto info = Core::CoreInterface::actionRegistry()->actionInfo(id);
        auto t = info.text(true);
        return t.isEmpty() ? info.text(false) : t;
    }
    QUrl AdditionalTrackLoader::componentIcon(const QString &id) {
        auto icon = Core::CoreInterface::actionRegistry()->actionIcon("", id);
        return icon.url();
    }

    void AdditionalTrackLoader::updateComponentIdList() {
        m_components.clear();
        auto actions = Core::CoreInterface::actionRegistry()->catalog().children(m_categoryId);
        m_components = actions;
    }

    QQuickItem *AdditionalTrackLoader::createItem(const QString &id) {
        Q_UNUSED(id)
        if (!m_contextObject)
            return nullptr;
        auto windowInterface = m_contextObject->property("windowHandle").value<Core::ProjectWindowInterface *>();
        Q_ASSERT(windowInterface);
        auto component = windowInterface->actionContext()->action(id);
        if (!component) {
            qCWarning(lcAdditionalTrackLoader) << "Component" << id << "not found";
            return nullptr;
        }
        std::unique_ptr<QObject> object(component->createWithInitialProperties({
            {"contextObject", QVariant::fromValue(m_contextObject)},
        }, component->creationContext()));
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
        return item;
    }

}

#include "moc_AdditionalTrackLoader.cpp"
