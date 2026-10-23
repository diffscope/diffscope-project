// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLOT_H
#define DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLOT_H

#include <array>

#include <QColor>
#include <QLineF>
#include <QPointer>
#include <QQuickItem>
#include <QVector>
#include <qqmlintegration.h>

#include <synth/internal/TempoExtractionPreview.h>

class QPainter;

namespace Synth::Internal {

    class TempoExtractionRasterPlot;

    class TempoExtractionPlot : public QQuickItem {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(TempoExtractionPreview *preview READ preview WRITE setPreview NOTIFY previewChanged)
        Q_PROPERTY(Plot plot READ plot WRITE setPlot NOTIFY appearanceChanged)
        Q_PROPERTY(bool candidateVisible READ candidateVisible WRITE setCandidateVisible NOTIFY appearanceChanged)
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY appearanceChanged)
        Q_PROPERTY(QColor detectedColor READ detectedColor WRITE setDetectedColor NOTIFY appearanceChanged)
        Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY appearanceChanged)
        Q_PROPERTY(double minimum READ minimum NOTIFY axisRangeChanged)
        Q_PROPERTY(double maximum READ maximum NOTIFY axisRangeChanged)
    public:
        enum Plot { Beats, Tempo, Deviation };
        Q_ENUM(Plot)

        explicit TempoExtractionPlot(QQuickItem *parent = nullptr);
        ~TempoExtractionPlot() override;
        TempoExtractionPreview *preview() const;
        void setPreview(TempoExtractionPreview *preview);
        Plot plot() const;
        void setPlot(Plot plot);
        bool candidateVisible() const;
        void setCandidateVisible(bool value);
        QColor color() const;
        void setColor(const QColor &value);
        QColor detectedColor() const;
        void setDetectedColor(const QColor &value);
        QColor gridColor() const;
        void setGridColor(const QColor &value);
        double minimum() const;
        double maximum() const;

    Q_SIGNALS:
        void previewChanged();
        void appearanceChanged();
        void axisRangeChanged();

    protected:
        void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
        void updatePolish() override;
        QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override;

    private:
        enum Layer { CurveLayer, DetectedLayer, GridLayer, LayerCount };
        friend class TempoExtractionRasterPlot;
        void invalidate();
        void selectRenderer();
        void paintSoftware(QPainter *painter);

        QPointer<TempoExtractionPreview> m_preview;
        TempoExtractionRasterPlot *m_raster{};
        Plot m_plot{Beats};
        bool m_candidateVisible{true};
        QColor m_color;
        QColor m_detectedColor;
        QColor m_gridColor;
        double m_minimum{};
        double m_maximum{1};
        std::array<QVector<QLineF>, LayerCount> m_lines;
    };

}

#endif // DIFFSCOPE_SYNTH_TEMPOEXTRACTIONPLOT_H
