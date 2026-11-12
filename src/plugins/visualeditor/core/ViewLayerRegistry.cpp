// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ViewLayerRegistry.h"
#include "ViewLayerRegistry_p.h"

#include <QLoggingCategory>
#include <QMetaEnum>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcViewLayerRegistry, "diffscope.visualeditor.viewlayerregistry")

    ViewLayerRegistry::ViewLayerRegistry(QObject *parent) : QObject(parent), d_ptr(new ViewLayerRegistryPrivate) {
        Q_D(ViewLayerRegistry);
        d->q_ptr = this;
    }

    ViewLayerRegistry::~ViewLayerRegistry() = default;

    ViewLayerRegistry *ViewLayerRegistry::instance() {
        return Core::RuntimeInterface::instance()->getFirstObject<ViewLayerRegistry>();
    }

    bool ViewLayerRegistry::registerLayer(ViewLayerPoint point, QQmlComponent *component) {
        if (!QMetaEnum::fromType<ViewLayerPoint>().valueToKey(point) || !component || !component->isReady()
            || component->engine() != Core::RuntimeInterface::qmlEngine()) {
            qCWarning(lcViewLayerRegistry) << "Could not register view layer" << point
                                         << (component ? component->url() : QUrl())
                                         << (component ? component->errorString() : QString());
            return false;
        }

        Q_D(ViewLayerRegistry);
        auto &components = d->components[point];
        if (components.contains(component))
            return true;

        components.append(component);
        connect(component, &QObject::destroyed, this, [this, point, component] {
            d_func()->components[point].removeAll(component);
            Q_EMIT layersChanged();
        });
        Q_EMIT layersChanged();
        return true;
    }

    QList<QQmlComponent *> ViewLayerRegistry::components(ViewLayerPoint point) const {
        Q_D(const ViewLayerRegistry);
        return d->components.value(point);
    }

}

#include "moc_ViewLayerRegistry.cpp"
