// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_VIEWLAYER_P_H
#define DIFFSCOPE_VISUALEDITOR_VIEWLAYER_P_H

#include <visualeditor/ViewLayer.h>

#include <QPointer>
#include <QRectF>

namespace Core {
    class ProjectWindowInterface;
}

namespace sflow {
    class ClipViewModel;
}

namespace VisualEditor {

    namespace Internal {
        class ViewLayerHost;
    }

    class ArrangementPanelInterface;
    class PianoRollPanelInterface;

    class ViewLayerTimeViewport {
        Q_GADGET
        QML_VALUE_TYPE(viewLayerTimeViewport)
        Q_PROPERTY(double visibleStartTick MEMBER visibleStartTick)
        Q_PROPERTY(double visibleEndTick MEMBER visibleEndTick)

    public:
        Q_INVOKABLE double xForTick(double tick) const {
            return (tick - visibleStartTick) * pixelDensity;
        }

        double visibleStartTick = 0;
        double visibleEndTick = 0;
        double pixelDensity = 0;
    };

    class ViewLayerAttachedType : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle NOTIFY windowHandleChanged)
        Q_PROPERTY(VisualEditor::ArrangementPanelInterface *arrangementPanelInterface READ arrangementPanelInterface NOTIFY arrangementPanelInterfaceChanged)
        Q_PROPERTY(VisualEditor::PianoRollPanelInterface *pianoRollPanelInterface READ pianoRollPanelInterface NOTIFY pianoRollPanelInterfaceChanged)
        Q_PROPERTY(sflow::ClipViewModel *clipViewModel READ clipViewModel NOTIFY clipViewModelChanged)
        Q_PROPERTY(QRectF visibleRect READ visibleRect NOTIFY visibleRectChanged)
        Q_PROPERTY(VisualEditor::ViewLayerTimeViewport timeViewport READ timeViewport NOTIFY timeViewportChanged)
        Q_MOC_INCLUDE("ScopicFlowCore/ClipViewModel.h")
        Q_MOC_INCLUDE("coreplugin/ProjectWindowInterface.h")
        Q_MOC_INCLUDE("visualeditor/ArrangementPanelInterface.h")
        Q_MOC_INCLUDE("visualeditor/PianoRollPanelInterface.h")

    public:
        explicit ViewLayerAttachedType(QObject *parent);
        Core::ProjectWindowInterface *windowHandle() const;
        ArrangementPanelInterface *arrangementPanelInterface() const;
        PianoRollPanelInterface *pianoRollPanelInterface() const;
        sflow::ClipViewModel *clipViewModel() const;
        QRectF visibleRect() const;
        ViewLayerTimeViewport timeViewport() const;

    Q_SIGNALS:
        void windowHandleChanged();
        void arrangementPanelInterfaceChanged();
        void pianoRollPanelInterfaceChanged();
        void clipViewModelChanged();
        void visibleRectChanged();
        void timeViewportChanged();

    private:
        friend class Internal::ViewLayerHost;
        void setHost(Internal::ViewLayerHost *host);

        QPointer<Internal::ViewLayerHost> m_host;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_VIEWLAYER_P_H
