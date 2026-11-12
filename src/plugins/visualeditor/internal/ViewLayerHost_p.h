// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_VIEWLAYERHOST_P_H
#define DIFFSCOPE_VISUALEDITOR_VIEWLAYERHOST_P_H

#include <QList>
#include <QPointer>
#include <QQuickItem>

#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/private/ViewLayer_p.h>

class QQmlComponent;

namespace VisualEditor::Internal {

    class ViewLayerHost : public QQuickItem {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(VisualEditor::ViewLayer::ViewLayerPoint point MEMBER m_point NOTIFY pointChanged REQUIRED)
        Q_PROPERTY(VisualEditor::ArrangementPanelInterface *arrangementPanelInterface READ arrangementPanelInterface WRITE setArrangementPanelInterface NOTIFY arrangementPanelInterfaceChanged)
        Q_PROPERTY(VisualEditor::PianoRollPanelInterface *pianoRollPanelInterface READ pianoRollPanelInterface WRITE setPianoRollPanelInterface NOTIFY pianoRollPanelInterfaceChanged)
        Q_PROPERTY(sflow::ClipViewModel *clipViewModel READ clipViewModel WRITE setClipViewModel NOTIFY clipViewModelChanged)
        Q_PROPERTY(QRectF visibleRect READ visibleRect MEMBER m_visibleRect NOTIFY visibleRectChanged)
        Q_PROPERTY(QList<QQuickItem *> items READ items NOTIFY itemsChanged)
        Q_MOC_INCLUDE("ScopicFlowCore/ClipViewModel.h")

    public:
        explicit ViewLayerHost(QQuickItem *parent = nullptr);

        Core::ProjectWindowInterface *windowHandle() const;
        ArrangementPanelInterface *arrangementPanelInterface() const;
        void setArrangementPanelInterface(ArrangementPanelInterface *panel);
        PianoRollPanelInterface *pianoRollPanelInterface() const;
        void setPianoRollPanelInterface(PianoRollPanelInterface *panel);
        sflow::ClipViewModel *clipViewModel() const;
        void setClipViewModel(sflow::ClipViewModel *clip);
        QRectF visibleRect() const;
        ViewLayerTimeViewport timeViewport() const;
        QList<QQuickItem *> items() const;

    Q_SIGNALS:
        void pointChanged();
        void windowHandleChanged();
        void arrangementPanelInterfaceChanged();
        void pianoRollPanelInterfaceChanged();
        void clipViewModelChanged();
        void visibleRectChanged();
        void timeViewportChanged();
        void itemsChanged();

    protected:
        void componentComplete() override;
        void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    private:
        struct Instance {
            QPointer<QQmlComponent> component;
            QPointer<QQuickItem> item;
        };

        void scheduleUpdate();
        void updateItems();
        void updateTimeModels();
        QQuickItem *createItem(QQmlComponent *component);

        ViewLayerPoint m_point = ViewLayerPoint::PianoRollBackground;
        QPointer<ArrangementPanelInterface> m_arrangementPanelInterface;
        QPointer<PianoRollPanelInterface> m_pianoRollPanelInterface;
        QPointer<sflow::ClipViewModel> m_clipViewModel;
        QRectF m_visibleRect;
        QList<Instance> m_instances;
        QPointer<sflow::TimeViewModel> m_timeViewModel;
        QPointer<sflow::TimeLayoutViewModel> m_timeLayoutViewModel;
        QMetaObject::Connection m_startConnection;
        QMetaObject::Connection m_pixelDensityConnection;
        bool m_updateScheduled = false;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_VIEWLAYERHOST_P_H
