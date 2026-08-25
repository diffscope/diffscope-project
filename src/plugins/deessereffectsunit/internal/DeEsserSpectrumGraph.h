// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERSPECTRUMGRAPH_H
#define DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERSPECTRUMGRAPH_H

#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <qqmlintegration.h>

namespace DeEsserEffectsUnit::Internal {

    class DeEsserEffectsUnit;

    class DeEsserSpectrumGraph : public QQuickItem {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(DeEsserEffectsUnit *effectsUnit READ effectsUnit WRITE setEffectsUnit NOTIFY effectsUnitChanged)
        Q_PROPERTY(QColor spectrumColor READ spectrumColor WRITE setSpectrumColor NOTIFY spectrumColorChanged)

    public:
        explicit DeEsserSpectrumGraph(QQuickItem *parent = nullptr);
        ~DeEsserSpectrumGraph() override;

        DeEsserEffectsUnit *effectsUnit() const;
        void setEffectsUnit(DeEsserEffectsUnit *effectsUnit);
        QColor spectrumColor() const;
        void setSpectrumColor(const QColor &color);

    Q_SIGNALS:
        void effectsUnitChanged();
        void spectrumColorChanged();

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode,
                                 UpdatePaintNodeData *updatePaintNodeData) override;
        void geometryChange(const QRectF &newGeometry,
                            const QRectF &oldGeometry) override;

    private:
        QPointer<DeEsserEffectsUnit> m_effectsUnit;
        QColor m_spectrumColor;
    };

}

#endif // DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSERSPECTRUMGRAPH_H
