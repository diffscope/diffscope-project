// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TempoExtractionPlot.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include <QPainter>
#include <QQuickPaintedItem>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGRendererInterface>

namespace Synth::Internal {

    class TempoExtractionRasterPlot : public QQuickPaintedItem {
    public:
        explicit TempoExtractionRasterPlot(TempoExtractionPlot *owner) : QQuickPaintedItem(owner), m_owner(owner) {
            setAntialiasing(true);
            setOpaquePainting(false);
            setAcceptedMouseButtons(Qt::NoButton);
        }

        void paint(QPainter *painter) override { m_owner->paintSoftware(painter); }

    private:
        TempoExtractionPlot *m_owner;
    };

    TempoExtractionPlot::TempoExtractionPlot(QQuickItem *parent) : QQuickItem(parent) {
        setFlag(ItemHasContents);
        setClip(true);
        connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *window) {
            if (window)
                connect(window, &QQuickWindow::sceneGraphInitialized, this, &TempoExtractionPlot::selectRenderer, Qt::QueuedConnection);
            selectRenderer();
        });
    }

    TempoExtractionPlot::~TempoExtractionPlot() = default;
    TempoExtractionPreview *TempoExtractionPlot::preview() const { return m_preview; }
    void TempoExtractionPlot::setPreview(TempoExtractionPreview *preview) {
        if (m_preview == preview)
            return;
        if (m_preview)
            disconnect(m_preview, nullptr, this, nullptr);
        m_preview = preview;
        if (preview) {
            connect(preview, &TempoExtractionPreview::changed, this, &TempoExtractionPlot::invalidate);
            connect(preview, &TempoExtractionPreview::rangeChanged, this, &TempoExtractionPlot::invalidate);
            connect(preview, &QObject::destroyed, this, &TempoExtractionPlot::invalidate);
        }
        Q_EMIT previewChanged();
        invalidate();
    }

    TempoExtractionPlot::Plot TempoExtractionPlot::plot() const { return m_plot; }
    void TempoExtractionPlot::setPlot(Plot value) {
        if (m_plot == value)
            return;
        m_plot = value;
        Q_EMIT appearanceChanged();
        invalidate();
    }

    bool TempoExtractionPlot::candidateVisible() const { return m_candidateVisible; }
    void TempoExtractionPlot::setCandidateVisible(bool value) {
        if (m_candidateVisible == value)
            return;
        m_candidateVisible = value;
        Q_EMIT appearanceChanged();
        invalidate();
    }

    QColor TempoExtractionPlot::color() const { return m_color; }
    void TempoExtractionPlot::setColor(const QColor &value) {
        if (m_color == value)
            return;
        m_color = value;
        Q_EMIT appearanceChanged();
        invalidate();
    }

    QColor TempoExtractionPlot::detectedColor() const { return m_detectedColor; }
    void TempoExtractionPlot::setDetectedColor(const QColor &value) {
        if (m_detectedColor == value)
            return;
        m_detectedColor = value;
        Q_EMIT appearanceChanged();
        invalidate();
    }

    QColor TempoExtractionPlot::gridColor() const { return m_gridColor; }
    void TempoExtractionPlot::setGridColor(const QColor &value) {
        if (m_gridColor == value)
            return;
        m_gridColor = value;
        Q_EMIT appearanceChanged();
        invalidate();
    }

    double TempoExtractionPlot::minimum() const { return m_minimum; }
    double TempoExtractionPlot::maximum() const { return m_maximum; }

    void TempoExtractionPlot::invalidate() {
        polish();
        update();
    }

    void TempoExtractionPlot::selectRenderer() {
        const bool software = window() && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        if (software && !m_raster)
            m_raster = new TempoExtractionRasterPlot(this);
        if (m_raster) {
            m_raster->setVisible(software);
            m_raster->setSize(size());
        }
        invalidate();
    }

    void TempoExtractionPlot::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
        QQuickItem::geometryChange(newGeometry, oldGeometry);
        if (m_raster)
            m_raster->setSize(newGeometry.size());
        invalidate();
    }

    void TempoExtractionPlot::updatePolish() {
        for (auto &lines : m_lines)
            lines.clear();
        if (m_raster)
            m_raster->update();
        if (!m_preview || !std::isfinite(width()) || !std::isfinite(height()) || width() <= 0 || height() <= 0)
            return;
        const auto start = m_preview->viewStart();
        const auto end = m_preview->viewEnd();
        if (!std::isfinite(start) || !std::isfinite(end) || end <= start)
            return;
        const auto x = [&](double seconds) { return (seconds - start) / (end - start) * width(); };
        const auto drawBeats = [&](const QVector<QPointF> &beats) {
            auto it = std::lower_bound(beats.cbegin(), beats.cend(), start, [](const QPointF &beat, double time) { return beat.x() < time; });
            int previousPixel = -1;
            for (; it != beats.cend() && it->x() <= end; ++it) {
                const auto pixel = qFloor(x(it->x()));
                if (pixel == previousPixel)
                    continue;
                previousPixel = pixel;
                m_lines[CurveLayer].append(QLineF(pixel + 0.5, 12, pixel + 0.5, height()));
            }
        };
        if (m_plot == Beats) {
            const auto &source = m_preview->input().beats;
            auto it = std::lower_bound(source.cbegin(), source.cend(), start - m_preview->begin(), [](const ExtractionBeat &beat, double time) { return beat.seconds < time; });
            int previousPixel = -1;
            for (; it != source.cend() && it->seconds <= end - m_preview->begin(); ++it) {
                const auto pixel = qFloor(x(m_preview->begin() + it->seconds));
                if (pixel == previousPixel && !it->downbeat)
                    continue;
                previousPixel = pixel;
                m_lines[DetectedLayer].append(QLineF(pixel, 0, pixel, it->downbeat ? 11 : 5));
                if (it->downbeat)
                    m_lines[DetectedLayer].append(QLineF(pixel + 1, 0, pixel + 1, 11));
            }
            const auto selected = m_preview->referenceBeat();
            if (selected >= 0 && selected < source.size()) {
                const auto selectedX = x(m_preview->begin() + source[selected].seconds);
                if (selectedX >= 0 && selectedX <= width()) {
                    m_lines[DetectedLayer].append(QLineF(selectedX - 4, 14, selectedX, 19));
                    m_lines[DetectedLayer].append(QLineF(selectedX, 19, selectedX + 4, 14));
                }
            }
            if (m_candidateVisible)
                drawBeats(m_preview->plan().candidateBeats);
        } else {
            const auto &points = m_plot == Tempo ? m_preview->plan().tempoPoints : m_preview->plan().errors;
            auto first = std::lower_bound(points.cbegin(), points.cend(), start, [](const QPointF &point, double time) { return point.x() < time; });
            auto last = std::upper_bound(points.cbegin(), points.cend(), end, [](double time, const QPointF &point) { return time < point.x(); });
            if (first != points.cbegin())
                --first;
            if (last != points.cend())
                ++last;
            double minimum = m_plot == Tempo ? 1000 : 0;
            double maximum = m_plot == Tempo ? 10 : 0;
            for (auto it = first; it != last; ++it) {
                if (!std::isfinite(it->x()) || !std::isfinite(it->y()))
                    continue;
                minimum = std::min(minimum, it->y());
                maximum = std::max(maximum, it->y());
            }
            if (minimum > maximum || !std::isfinite(maximum - minimum)) {
                minimum = 0;
                maximum = 1;
            }
            const auto padding = std::max(1.0, (maximum - minimum) * 0.1);
            minimum -= padding;
            maximum += padding;
            if (minimum != m_minimum || maximum != m_maximum) {
                m_minimum = minimum;
                m_maximum = maximum;
                Q_EMIT axisRangeChanged();
            }
            const auto y = [&](double value) { return height() - (value - minimum) / (maximum - minimum) * height(); };
            if (m_plot == Deviation) {
                m_lines[GridLayer].append(QLineF(0, y(0), width(), y(0)));
            }
            // Each pixel column retains endpoints and extrema in source order.
            QVector<QPointF> reduced;
            auto bucket = first;
            while (bucket != last) {
                if (!std::isfinite(bucket->x()) || !std::isfinite(bucket->y()) || !std::isfinite(x(bucket->x()))) {
                    ++bucket;
                    continue;
                }
                auto finish = std::next(bucket);
                auto low = bucket;
                auto high = bucket;
                const auto column = std::floor(std::clamp(x(bucket->x()), -1.0, width() + 1));
                while (finish != last && std::isfinite(finish->x()) && std::isfinite(finish->y())
                       && std::isfinite(x(finish->x())) && std::floor(std::clamp(x(finish->x()), -1.0, width() + 1)) == column) {
                    if (finish->y() < low->y())
                        low = finish;
                    if (finish->y() > high->y())
                        high = finish;
                    ++finish;
                }
                std::array selected{bucket, low, high, std::prev(finish)};
                std::sort(selected.begin(), selected.end());
                const auto uniqueEnd = std::unique(selected.begin(), selected.end());
                for (auto it = selected.begin(); it != uniqueEnd; ++it)
                    reduced.append(**it);
                bucket = finish;
            }
            for (qsizetype i = 1; i < reduced.size(); ++i) {
                QPointF a(x(reduced[i - 1].x()), y(reduced[i - 1].y()));
                QPointF b(x(reduced[i].x()), y(reduced[i].y()));
                if (!std::isfinite(a.x()) || !std::isfinite(a.y()) || !std::isfinite(b.x()) || !std::isfinite(b.y()))
                    continue;
                if (a.x() < 0 && b.x() > a.x())
                    a = {0, a.y() + (b.y() - a.y()) * -a.x() / (b.x() - a.x())};
                if (b.x() > width() && b.x() > a.x())
                    b = {width(), a.y() + (b.y() - a.y()) * (width() - a.x()) / (b.x() - a.x())};
                if (a.x() <= width() && b.x() >= 0)
                    m_lines[CurveLayer].append(QLineF(a, b));
            }
        }
    }

    void TempoExtractionPlot::paintSoftware(QPainter *painter) {
        painter->setClipRect(boundingRect());
        const std::array colors{m_color, m_detectedColor, m_gridColor};
        for (int i = LayerCount - 1; i >= 0; --i) {
            painter->setPen(QPen(colors[i], 1));
            painter->drawLines(m_lines[i]);
        }
    }

    QSGNode *TempoExtractionPlot::updatePaintNode(QSGNode *node, UpdatePaintNodeData *) {
        if (!window() || window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
            if (window() && !m_raster)
                QMetaObject::invokeMethod(this, &TempoExtractionPlot::selectRenderer, Qt::QueuedConnection);
            delete node;
            return nullptr;
        }
        if (!node) {
            node = new QSGNode;
            for (int i = 0; i < LayerCount; ++i) {
                auto child = new QSGGeometryNode;
                auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
                geometry->setDrawingMode(QSGGeometry::DrawTriangles);
                geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
                child->setGeometry(geometry);
                child->setFlag(QSGNode::OwnsGeometry);
                child->setMaterial(new QSGFlatColorMaterial);
                child->setFlag(QSGNode::OwnsMaterial);
                node->appendChildNode(child);
            }
        }
        const std::array colors{m_color, m_detectedColor, m_gridColor};
        auto child = node->firstChild();
        for (int layer = LayerCount - 1; layer >= 0; --layer, child = child->nextSibling()) {
            auto geometryNode = static_cast<QSGGeometryNode *>(child);
            auto geometry = geometryNode->geometry();
            geometry->allocate(static_cast<int>(m_lines[layer].size() * 6));
            auto vertices = geometry->vertexDataAsPoint2D();
            for (const auto &line : m_lines[layer]) {
                const auto length = line.length();
                const auto normal = length > 0 ? QPointF(-line.dy(), line.dx()) / (2 * length) : QPointF(0.5, 0);
                const std::array corners{line.p1() + normal, line.p1() - normal, line.p2() + normal, line.p2() - normal};
                for (const auto index : {0, 1, 2, 2, 1, 3}) {
                    vertices->set(corners[index].x(), corners[index].y());
                    ++vertices;
                }
            }
            static_cast<QSGFlatColorMaterial *>(geometryNode->material())->setColor(colors[layer]);
            geometryNode->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
        }
        return node;
    }

}

#include "moc_TempoExtractionPlot.cpp"
