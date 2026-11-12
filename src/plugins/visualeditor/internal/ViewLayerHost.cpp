// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ViewLayerHost_p.h"

#include <algorithm>

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <qqml.h>

#include <ScopicFlowCore/ClipViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>

#include <visualeditor/ViewLayerRegistry.h>

namespace VisualEditor::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcViewLayerHost, "diffscope.visualeditor.viewlayerhost")

    ViewLayerHost::ViewLayerHost(QQuickItem *parent) : QQuickItem(parent) {
        connect(this, &ViewLayerHost::pointChanged, this, &ViewLayerHost::scheduleUpdate);
        connect(this, &ViewLayerHost::pointChanged, this, &ViewLayerHost::updateTimeModels);
        connect(this, &ViewLayerHost::arrangementPanelInterfaceChanged, this, &ViewLayerHost::updateTimeModels);
        connect(this, &ViewLayerHost::pianoRollPanelInterfaceChanged, this, &ViewLayerHost::updateTimeModels);
    }

    Core::ProjectWindowInterface *ViewLayerHost::windowHandle() const {
        if (m_arrangementPanelInterface)
            return m_arrangementPanelInterface->windowHandle();
        return m_pianoRollPanelInterface ? m_pianoRollPanelInterface->windowHandle() : nullptr;
    }

    ArrangementPanelInterface *ViewLayerHost::arrangementPanelInterface() const {
        return m_arrangementPanelInterface;
    }

    void ViewLayerHost::setArrangementPanelInterface(ArrangementPanelInterface *panel) {
        if (m_arrangementPanelInterface == panel)
            return;
        m_arrangementPanelInterface = panel;
        Q_EMIT arrangementPanelInterfaceChanged();
    }

    PianoRollPanelInterface *ViewLayerHost::pianoRollPanelInterface() const {
        return m_pianoRollPanelInterface;
    }

    void ViewLayerHost::setPianoRollPanelInterface(PianoRollPanelInterface *panel) {
        if (m_pianoRollPanelInterface == panel)
            return;
        m_pianoRollPanelInterface = panel;
        Q_EMIT pianoRollPanelInterfaceChanged();
    }

    sflow::ClipViewModel *ViewLayerHost::clipViewModel() const {
        return m_clipViewModel;
    }

    void ViewLayerHost::setClipViewModel(sflow::ClipViewModel *clip) {
        if (m_clipViewModel == clip)
            return;
        m_clipViewModel = clip;
        Q_EMIT clipViewModelChanged();
    }

    QRectF ViewLayerHost::visibleRect() const {
        return m_visibleRect;
    }

    ViewLayerTimeViewport ViewLayerHost::timeViewport() const {
        if (!m_timeViewModel || !m_timeLayoutViewModel || m_timeLayoutViewModel->pixelDensity() <= 0)
            return {};
        const auto start = m_timeViewModel->start();
        const auto pixelDensity = m_timeLayoutViewModel->pixelDensity();
        return {start, start + width() / pixelDensity, pixelDensity};
    }

    QList<QQuickItem *> ViewLayerHost::items() const {
        QList<QQuickItem *> items;
        for (const auto &instance : m_instances) {
            if (instance.item)
                items.append(instance.item);
        }
        return items;
    }

    void ViewLayerHost::componentComplete() {
        QQuickItem::componentComplete();
        if (auto registry = ViewLayerRegistry::instance())
            connect(registry, &ViewLayerRegistry::layersChanged, this, &ViewLayerHost::scheduleUpdate);
        scheduleUpdate();
    }

    void ViewLayerHost::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
        QQuickItem::geometryChange(newGeometry, oldGeometry);
        if (newGeometry.size() != oldGeometry.size()) {
            for (const auto &instance : m_instances) {
                if (instance.item)
                    instance.item->setSize(newGeometry.size());
            }
        }
        if (newGeometry.width() != oldGeometry.width())
            Q_EMIT timeViewportChanged();
    }

    void ViewLayerHost::scheduleUpdate() {
        if (!isComponentComplete() || m_updateScheduled)
            return;
        m_updateScheduled = true;
        QMetaObject::invokeMethod(this, &ViewLayerHost::updateItems, Qt::QueuedConnection);
    }

    void ViewLayerHost::updateItems() {
        m_updateScheduled = false;
        auto registry = ViewLayerRegistry::instance();
        if (!registry)
            return;

        const auto components = registry->components(m_point);
        bool changed = m_instances.removeIf([&components](const Instance &instance) {
            if (instance.component && components.contains(instance.component))
                return false;
            if (instance.item) {
                instance.item->setEnabled(false);
                instance.item->setParentItem(nullptr);
                instance.item->deleteLater();
            }
            return true;
        }) > 0;
        for (auto component : components) {
            if (std::ranges::any_of(m_instances, [component](const Instance &instance) {
                    return instance.component == component;
                }))
                continue;
            auto item = createItem(component);
            m_instances.append({component, item});
            changed |= item != nullptr;
        }
        if (changed)
            Q_EMIT itemsChanged();
    }

    void ViewLayerHost::updateTimeModels() {
        disconnect(m_startConnection);
        disconnect(m_pixelDensityConnection);
        if (m_point == ViewLayerPoint::ArrangementClipContent || m_point == ViewLayerPoint::ArrangementClipAdornment
            || m_point == ViewLayerPoint::PianoRollKeyboardForeground) {
            m_timeViewModel = nullptr;
            m_timeLayoutViewModel = nullptr;
        } else if (m_arrangementPanelInterface) {
            m_timeViewModel = m_arrangementPanelInterface->timeViewModel();
            m_timeLayoutViewModel = m_arrangementPanelInterface->timeLayoutViewModel();
        } else {
            m_timeViewModel = m_pianoRollPanelInterface ? m_pianoRollPanelInterface->timeViewModel() : nullptr;
            m_timeLayoutViewModel = m_pianoRollPanelInterface ? m_pianoRollPanelInterface->timeLayoutViewModel() : nullptr;
        }
        if (m_timeViewModel)
            m_startConnection = connect(m_timeViewModel.data(), &sflow::TimeViewModel::startChanged, this, &ViewLayerHost::timeViewportChanged);
        if (m_timeLayoutViewModel)
            m_pixelDensityConnection = connect(m_timeLayoutViewModel.data(), &sflow::TimeLayoutViewModel::pixelDensityChanged, this, &ViewLayerHost::timeViewportChanged);
        Q_EMIT windowHandleChanged();
        Q_EMIT timeViewportChanged();
    }

    QQuickItem *ViewLayerHost::createItem(QQmlComponent *component) {
        auto context = component->creationContext();
        if (!context)
            context = qmlContext(this);
        if (!context || !context->isValid())
            return nullptr;
        QPointer<QObject> object = component->beginCreate(context);
        if (!object) {
            qCWarning(lcViewLayerHost) << "Could not create view layer" << m_point << component->url() << component->errorString();
            return nullptr;
        }

        auto item = qobject_cast<QQuickItem *>(object.data());
        auto attached = item ? qobject_cast<ViewLayerAttachedType *>(qmlAttachedPropertiesObject<ViewLayer>(item)) : nullptr;
        if (attached) {
            attached->setHost(this);
            item->setParent(this);
            item->setParentItem(this);
            item->setSize(size());
        }
        component->completeCreate();
        if (!object || !attached || component->isError()) {
            qCWarning(lcViewLayerHost) << "Invalid view layer" << m_point << component->url() << component->errorString();
            delete object.data();
            return nullptr;
        }
        return item;
    }

}

#include "moc_ViewLayerHost_p.cpp"
