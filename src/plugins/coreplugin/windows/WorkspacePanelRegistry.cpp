// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspacePanelRegistry.h"
#include "WorkspacePanelRegistry_p.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

namespace Core {

    WorkspacePanelRegistry::WorkspacePanelRegistry(QObject *parent) : QObject(parent), d_ptr(new WorkspacePanelRegistryPrivate) {
        Q_D(WorkspacePanelRegistry);
        d->q_ptr = this;
    }

    WorkspacePanelRegistry::~WorkspacePanelRegistry() = default;

    bool WorkspacePanelRegistry::registerPanel(const QString &id, QQmlComponent *component, const Metadata &metadata) {
        Q_D(WorkspacePanelRegistry);
        if (id.isEmpty() || !component || component->engine() != RuntimeInterface::qmlEngine() || d->entries.contains(id))
            return false;

        d->entries.insert(id, {
            component,
            metadata,
            connect(component, &QObject::destroyed, this, [this, id] { unregisterPanel(id); }),
            connect(component, &QQmlComponent::statusChanged, this, [this, id] { Q_EMIT componentStatusChanged(id); }),
        });
        d->ids.append(id);
        Q_EMIT registrationChanged(id);
        return true;
    }

    bool WorkspacePanelRegistry::unregisterPanel(const QString &id) {
        Q_D(WorkspacePanelRegistry);
        if (!d->entries.contains(id))
            return false;

        auto entry = d->entries.take(id);
        disconnect(entry.destroyedConnection);
        disconnect(entry.statusConnection);
        d->ids.removeAll(id);
        Q_EMIT registrationChanged(id);
        return true;
    }

    QStringList WorkspacePanelRegistry::panelIds() const {
        Q_D(const WorkspacePanelRegistry);
        return d->ids;
    }

    QQmlComponent *WorkspacePanelRegistry::component(const QString &id) const {
        Q_D(const WorkspacePanelRegistry);
        return d->entries.value(id).component;
    }

    WorkspacePanelRegistry::Metadata WorkspacePanelRegistry::metadata(const QString &id) const {
        Q_D(const WorkspacePanelRegistry);
        return d->entries.value(id).metadata;
    }

}

#include "moc_WorkspacePanelRegistry.cpp"
