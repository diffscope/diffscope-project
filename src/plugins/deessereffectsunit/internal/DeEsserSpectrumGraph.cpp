// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeEsserSpectrumGraph.h"

#include <algorithm>
#include <array>

#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>

#include <SVSCraftQuick/SoftwarePainterNode.h>

#include <deessereffectsunit/internal/DeEsserEffectsUnit.h>
#include <deessereffectsunit/internal/DeEsserParameters.h>

namespace DeEsserEffectsUnit::Internal {

    namespace {

        constexpr int spectrumFillVertexCount = deEsserSpectrumBinCount * 2;

        float yFromSpectrum(float spectrumDb, float height);

        QSGGeometryNode *createGeometryNode(int vertexCount,
                                            QSGGeometry::DrawingMode drawingMode) {
            auto node = new QSGGeometryNode;
            auto geometry = new QSGGeometry(
                QSGGeometry::defaultAttributes_Point2D(), vertexCount);
            geometry->setDrawingMode(drawingMode);
            geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
            auto material = new QSGFlatColorMaterial;
            node->setMaterial(material);
            node->setFlag(QSGNode::OwnsMaterial);
            return node;
        }

        class DeEsserSpectrumGraphNode : public QSGNode {
        public:
            DeEsserSpectrumGraphNode()
                : spectrumFill(createGeometryNode(spectrumFillVertexCount,
                                                  QSGGeometry::DrawTriangleStrip)),
                  spectrumLine(createGeometryNode(deEsserSpectrumBinCount,
                                                  QSGGeometry::DrawLineStrip)) {
                appendChildNode(spectrumFill);
                appendChildNode(spectrumLine);
            }

            QSGGeometryNode *spectrumFill;
            QSGGeometryNode *spectrumLine;
        };

        class DeEsserSpectrumGraphSoftwareNode : public SVS::SoftwarePainterNode {
        public:
            explicit DeEsserSpectrumGraphSoftwareNode(QQuickItem *item) : SoftwarePainterNode(item) {
            }

            void synchronize(const std::array<float, deEsserSpectrumBinCount> &spectrum,
                             float width,
                             float height,
                             const QColor &color) {
                const QSizeF size(width, height);
                if (m_size != size || m_spectrum != spectrum) {
                    m_spectrum = spectrum;
                    m_fillPath = {};
                    m_linePath = {};
                    if (!spectrum.empty()) {
                        m_fillPath.moveTo(0, height);
                        for (int index = 0; index < deEsserSpectrumBinCount; ++index) {
                            const float x = width * static_cast<float>(index)
                                / static_cast<float>(deEsserSpectrumBinCount - 1);
                            const QPointF point(x, yFromSpectrum(
                                spectrum.at(static_cast<std::size_t>(index)), height));
                            if (index == 0) {
                                m_linePath.moveTo(point);
                            } else {
                                m_linePath.lineTo(point);
                            }
                            m_fillPath.lineTo(point);
                        }
                        m_fillPath.lineTo(width, height);
                        m_fillPath.closeSubpath();
                    }
                    m_size = size;
                    markDirty(QSGNode::DirtyGeometry);
                }
                if (m_color != color) {
                    m_color = color;
                    markDirty(QSGNode::DirtyMaterial);
                }
                setBoundingRect(QRectF(QPointF(), size).adjusted(-1, -1, 1, 1));
            }

        protected:
            void paint(QPainter *painter) override {
                painter->setRenderHint(QPainter::Antialiasing);
                painter->fillPath(m_fillPath, m_color);
                QColor lineColor = m_color;
                lineColor.setAlphaF(std::min(1.0, lineColor.alphaF() * 2.0));
                painter->setBrush(Qt::NoBrush);
                QPen pen(lineColor, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
                pen.setCosmetic(true);
                painter->setPen(pen);
                painter->drawPath(m_linePath);
            }

        private:
            std::array<float, deEsserSpectrumBinCount> m_spectrum{};
            QPainterPath m_fillPath;
            QPainterPath m_linePath;
            QSizeF m_size;
            QColor m_color;
        };

        void setMaterialColor(QSGGeometryNode *node, const QColor &color) {
            auto material = static_cast<QSGFlatColorMaterial *>(node->material());
            material->setColor(color);
            node->markDirty(QSGNode::DirtyMaterial);
        }

        float yFromSpectrum(float spectrumDb, float height) {
            const float normalized = (std::clamp(spectrumDb, meterFloorDb, 0.0f)
                                      - meterFloorDb)
                / -meterFloorDb;
            return height * (1.0f - normalized);
        }

    }

    DeEsserSpectrumGraph::DeEsserSpectrumGraph(QQuickItem *parent)
        : QQuickItem(parent) {
        setFlag(ItemHasContents, true);
    }

    DeEsserSpectrumGraph::~DeEsserSpectrumGraph() = default;

    DeEsserEffectsUnit *DeEsserSpectrumGraph::effectsUnit() const {
        return m_effectsUnit;
    }

    void DeEsserSpectrumGraph::setEffectsUnit(DeEsserEffectsUnit *effectsUnit) {
        if (m_effectsUnit == effectsUnit) {
            return;
        }
        if (m_effectsUnit) {
            disconnect(m_effectsUnit, nullptr, this, nullptr);
        }
        m_effectsUnit = effectsUnit;
        if (m_effectsUnit) {
            connect(m_effectsUnit, &DeEsserEffectsUnit::spectrumCurveChanged,
                    this, [this] { update(); });
        }
        update();
        Q_EMIT effectsUnitChanged();
    }

    QColor DeEsserSpectrumGraph::spectrumColor() const {
        return m_spectrumColor;
    }

    void DeEsserSpectrumGraph::setSpectrumColor(const QColor &color) {
        if (m_spectrumColor == color) {
            return;
        }
        m_spectrumColor = color;
        update();
        Q_EMIT spectrumColorChanged();
    }

    QSGNode *DeEsserSpectrumGraph::updatePaintNode(
        QSGNode *oldNode, UpdatePaintNodeData *) {
        const float graphWidth = static_cast<float>(width());
        const float graphHeight = static_cast<float>(height());
        auto spectrum = m_effectsUnit
            ? m_effectsUnit->spectrumCurveDb()
            : std::array<float, deEsserSpectrumBinCount>{};
        if (!m_effectsUnit) {
            spectrum.fill(meterFloorDb);
        }

        const bool software = window()
            && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        if (software) {
            auto *node = dynamic_cast<DeEsserSpectrumGraphSoftwareNode *>(oldNode);
            if (!node) {
                delete oldNode;
                node = new DeEsserSpectrumGraphSoftwareNode(this);
            }
            node->synchronize(spectrum, graphWidth, graphHeight, m_spectrumColor);
            return node;
        }

        auto node = dynamic_cast<DeEsserSpectrumGraphNode *>(oldNode);
        if (!node) {
            delete oldNode;
            node = new DeEsserSpectrumGraphNode;
        }

        auto fillVertices = node->spectrumFill->geometry()->vertexDataAsPoint2D();
        auto spectrumVertices = node->spectrumLine->geometry()->vertexDataAsPoint2D();
        for (int index = 0; index < deEsserSpectrumBinCount; ++index) {
            const float x = graphWidth * static_cast<float>(index)
                / static_cast<float>(deEsserSpectrumBinCount - 1);
            const float y = yFromSpectrum(
                spectrum.at(static_cast<std::size_t>(index)), graphHeight);
            fillVertices[index * 2].set(x, graphHeight);
            fillVertices[index * 2 + 1].set(x, y);
            spectrumVertices[index].set(x, y);
        }
        node->spectrumFill->geometry()->markVertexDataDirty();
        node->spectrumFill->markDirty(QSGNode::DirtyGeometry);
        node->spectrumLine->geometry()->markVertexDataDirty();
        node->spectrumLine->markDirty(QSGNode::DirtyGeometry);

        setMaterialColor(node->spectrumFill, m_spectrumColor);
        QColor spectrumLineColor = m_spectrumColor;
        spectrumLineColor.setAlphaF(
            std::min(1.0, spectrumLineColor.alphaF() * 2.0));
        setMaterialColor(node->spectrumLine, spectrumLineColor);
        return node;
    }

    void DeEsserSpectrumGraph::geometryChange(const QRectF &newGeometry,
                                              const QRectF &oldGeometry) {
        QQuickItem::geometryChange(newGeometry, oldGeometry);
        if (newGeometry.size() != oldGeometry.size()) {
            update();
        }
    }

}

#include "moc_DeEsserSpectrumGraph.cpp"
