// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERGRAPH_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERGRAPH_H

#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <qqmlintegration.h>

namespace EqualizerEffectsUnit::Internal {

    class EqualizerEffectsUnit;

    class EqualizerGraph : public QQuickItem {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(EqualizerEffectsUnit *effectsUnit READ effectsUnit WRITE setEffectsUnit NOTIFY effectsUnitChanged)
        Q_PROPERTY(QColor guideColor READ guideColor WRITE setGuideColor NOTIFY guideColorChanged)
        Q_PROPERTY(QColor responseColor READ responseColor WRITE setResponseColor NOTIFY responseColorChanged)
        Q_PROPERTY(QColor spectrumColor READ spectrumColor WRITE setSpectrumColor NOTIFY spectrumColorChanged)

    public:
        explicit EqualizerGraph(QQuickItem *parent = nullptr);
        ~EqualizerGraph() override;

        EqualizerEffectsUnit *effectsUnit() const;
        void setEffectsUnit(EqualizerEffectsUnit *effectsUnit);

        QColor guideColor() const;
        void setGuideColor(const QColor &color);
        QColor responseColor() const;
        void setResponseColor(const QColor &color);
        QColor spectrumColor() const;
        void setSpectrumColor(const QColor &color);

    Q_SIGNALS:
        void effectsUnitChanged();
        void guideColorChanged();
        void responseColorChanged();
        void spectrumColorChanged();

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode,
                                 UpdatePaintNodeData *updatePaintNodeData) override;
        void geometryChange(const QRectF &newGeometry,
                            const QRectF &oldGeometry) override;

    private:
        QPointer<EqualizerEffectsUnit> m_effectsUnit;
        QColor m_guideColor;
        QColor m_responseColor;
        QColor m_spectrumColor;
    };

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZERGRAPH_H
