// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_H
#define DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_H

#include <QList>
#include <QObject>
#include <QScopedPointer>

#include <visualeditor/ViewLayer.h>
#include <visualeditor/visualeditorglobal.h>

class QQmlComponent;

namespace VisualEditor {

    namespace Internal {
        class ViewLayerHost;
        class VisualEditorPlugin;
    }

    class ViewLayerRegistryPrivate;

    class VISUAL_EDITOR_EXPORT ViewLayerRegistry : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ViewLayerRegistry)

    public:
        ~ViewLayerRegistry() override;

        static ViewLayerRegistry *instance();
        bool registerLayer(ViewLayerPoint point, QQmlComponent *component);

    Q_SIGNALS:
        void layersChanged();

    private:
        friend class Internal::ViewLayerHost;
        friend class Internal::VisualEditorPlugin;
        explicit ViewLayerRegistry(QObject *parent);
        QList<QQmlComponent *> components(ViewLayerPoint point) const;

        QScopedPointer<ViewLayerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_VIEWLAYERREGISTRY_H
