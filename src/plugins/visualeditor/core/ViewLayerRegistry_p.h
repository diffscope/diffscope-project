// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_P_H
#define DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_P_H

#include <visualeditor/ViewLayerRegistry.h>

#include <QHash>
#include <QList>

namespace VisualEditor {

    class ViewLayerRegistryPrivate {
        Q_DECLARE_PUBLIC(ViewLayerRegistry)
    public:
        ViewLayerRegistry *q_ptr = nullptr;
        QHash<ViewLayerPoint, QList<QQmlComponent *>> components;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_P_H
