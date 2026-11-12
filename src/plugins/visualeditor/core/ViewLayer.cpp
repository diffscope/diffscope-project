// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ViewLayer.h"
#include "ViewLayer_p.h"

#include <visualeditor/internal/private/ViewLayerHost_p.h>

namespace VisualEditor {

    ViewLayerAttachedType *ViewLayer::qmlAttachedProperties(QObject *object) {
        return new ViewLayerAttachedType(object);
    }

    ViewLayerAttachedType::ViewLayerAttachedType(QObject *parent) : QObject(parent) {
    }

    Core::ProjectWindowInterface *ViewLayerAttachedType::windowHandle() const {
        return m_host ? m_host->windowHandle() : nullptr;
    }

    ArrangementPanelInterface *ViewLayerAttachedType::arrangementPanelInterface() const {
        return m_host ? m_host->arrangementPanelInterface() : nullptr;
    }

    PianoRollPanelInterface *ViewLayerAttachedType::pianoRollPanelInterface() const {
        return m_host ? m_host->pianoRollPanelInterface() : nullptr;
    }

    sflow::ClipViewModel *ViewLayerAttachedType::clipViewModel() const {
        return m_host ? m_host->clipViewModel() : nullptr;
    }

    QRectF ViewLayerAttachedType::visibleRect() const {
        return m_host ? m_host->visibleRect() : QRectF{};
    }

    ViewLayerTimeViewport ViewLayerAttachedType::timeViewport() const {
        return m_host ? m_host->timeViewport() : ViewLayerTimeViewport{};
    }

    void ViewLayerAttachedType::setHost(Internal::ViewLayerHost *host) {
        m_host = host;
        connect(host, &Internal::ViewLayerHost::arrangementPanelInterfaceChanged, this, &ViewLayerAttachedType::arrangementPanelInterfaceChanged);
        connect(host, &Internal::ViewLayerHost::pianoRollPanelInterfaceChanged, this, &ViewLayerAttachedType::pianoRollPanelInterfaceChanged);
        connect(host, &Internal::ViewLayerHost::windowHandleChanged, this, &ViewLayerAttachedType::windowHandleChanged);
        connect(host, &Internal::ViewLayerHost::clipViewModelChanged, this, &ViewLayerAttachedType::clipViewModelChanged);
        connect(host, &Internal::ViewLayerHost::visibleRectChanged, this, &ViewLayerAttachedType::visibleRectChanged);
        connect(host, &Internal::ViewLayerHost::timeViewportChanged, this, &ViewLayerAttachedType::timeViewportChanged);
        Q_EMIT windowHandleChanged();
        Q_EMIT arrangementPanelInterfaceChanged();
        Q_EMIT pianoRollPanelInterfaceChanged();
        Q_EMIT clipViewModelChanged();
        Q_EMIT visibleRectChanged();
        Q_EMIT timeViewportChanged();
    }

}

#include "moc_ViewLayer.cpp"
#include "moc_ViewLayer_p.cpp"
