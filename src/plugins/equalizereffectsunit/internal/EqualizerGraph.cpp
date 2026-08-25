// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerGraph.h"

#include <algorithm>
#include <cmath>

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>

#include <equalizereffectsunit/internal/EqualizerEffectsUnit.h>
#include <equalizereffectsunit/internal/EqualizerParameters.h>

namespace EqualizerEffectsUnit::Internal {

    namespace {

        constexpr int guideVertexCount = 8;
        constexpr int spectrumFillVertexCount = equalizerSpectrumBinCount * 2;

        QSGGeometryNode *createGeometryNode(int vertexCount,
                                            QSGGeometry::DrawingMode drawingMode,
                                            float lineWidth = 1.0f) {
            auto node = new QSGGeometryNode;
            auto geometry = new QSGGeometry(
                QSGGeometry::defaultAttributes_Point2D(), vertexCount);
            geometry->setDrawingMode(drawingMode);
            geometry->setLineWidth(lineWidth);
            geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
            auto material = new QSGFlatColorMaterial;
            node->setMaterial(material);
            node->setFlag(QSGNode::OwnsMaterial);
            return node;
        }

        class EqualizerGraphNode : public QSGNode {
        public:
            EqualizerGraphNode()
                : guides(createGeometryNode(guideVertexCount,
                                            QSGGeometry::DrawLines)),
                  spectrumFill(createGeometryNode(spectrumFillVertexCount,
                                                  QSGGeometry::DrawTriangleStrip)),
                  spectrumLine(createGeometryNode(equalizerSpectrumBinCount,
                                                  QSGGeometry::DrawLineStrip)),
                  responseLine(createGeometryNode(equalizerResponsePointCount,
                                                  QSGGeometry::DrawLineStrip)) {
                appendChildNode(guides);
                appendChildNode(spectrumFill);
                appendChildNode(spectrumLine);
                appendChildNode(responseLine);
            }

            QSGGeometryNode *guides;
            QSGGeometryNode *spectrumFill;
            QSGGeometryNode *spectrumLine;
            QSGGeometryNode *responseLine;
        };

        void setMaterialColor(QSGGeometryNode *node, const QColor &color) {
            auto material = static_cast<QSGFlatColorMaterial *>(node->material());
            material->setColor(color);
            node->markDirty(QSGNode::DirtyMaterial);
        }

        float xFromFrequency(double frequency, float width) {
            const double position = std::log(frequency / minimumFrequencyHz)
                / std::log(maximumFrequencyHz / minimumFrequencyHz);
            return static_cast<float>(position) * width;
        }

        float yFromResponse(float responseDb, float height) {
            const float normalized = (std::clamp(responseDb,
                                                 static_cast<float>(minimumGainDb),
                                                 static_cast<float>(maximumGainDb))
                                      - static_cast<float>(minimumGainDb))
                / static_cast<float>(maximumGainDb - minimumGainDb);
            return height * (1.0f - normalized);
        }

        float yFromSpectrum(float spectrumDb, float height) {
            const float normalized = (std::clamp(spectrumDb, -96.0f, 0.0f) + 96.0f)
                / 96.0f;
            return height * (1.0f - normalized);
        }

    }

    EqualizerGraph::EqualizerGraph(QQuickItem *parent)
        : QQuickItem(parent) {
        setFlag(ItemHasContents, true);
    }

    EqualizerGraph::~EqualizerGraph() = default;

    EqualizerEffectsUnit *EqualizerGraph::effectsUnit() const {
        return m_effectsUnit;
    }

    void EqualizerGraph::setEffectsUnit(EqualizerEffectsUnit *effectsUnit) {
        if (m_effectsUnit == effectsUnit) {
            return;
        }
        if (m_effectsUnit) {
            disconnect(m_effectsUnit, nullptr, this, nullptr);
        }
        m_effectsUnit = effectsUnit;
        if (m_effectsUnit) {
            connect(m_effectsUnit, &EqualizerEffectsUnit::responseCurveChanged,
                    this, [this] { update(); });
            connect(m_effectsUnit, &EqualizerEffectsUnit::spectrumCurveChanged,
                    this, [this] { update(); });
        }
        update();
        Q_EMIT effectsUnitChanged();
    }

    QColor EqualizerGraph::guideColor() const {
        return m_guideColor;
    }

    void EqualizerGraph::setGuideColor(const QColor &color) {
        if (m_guideColor == color) {
            return;
        }
        m_guideColor = color;
        update();
        Q_EMIT guideColorChanged();
    }

    QColor EqualizerGraph::responseColor() const {
        return m_responseColor;
    }

    void EqualizerGraph::setResponseColor(const QColor &color) {
        if (m_responseColor == color) {
            return;
        }
        m_responseColor = color;
        update();
        Q_EMIT responseColorChanged();
    }

    QColor EqualizerGraph::spectrumColor() const {
        return m_spectrumColor;
    }

    void EqualizerGraph::setSpectrumColor(const QColor &color) {
        if (m_spectrumColor == color) {
            return;
        }
        m_spectrumColor = color;
        update();
        Q_EMIT spectrumColorChanged();
    }

    QSGNode *EqualizerGraph::updatePaintNode(
        QSGNode *oldNode, UpdatePaintNodeData *) {
        auto node = static_cast<EqualizerGraphNode *>(oldNode);
        if (!node) {
            node = new EqualizerGraphNode;
        }

        const float graphWidth = static_cast<float>(width());
        const float graphHeight = static_cast<float>(height());
        const auto response = m_effectsUnit
            ? m_effectsUnit->responseCurveDb()
            : std::array<float, equalizerResponsePointCount>{};
        auto spectrum = m_effectsUnit
            ? m_effectsUnit->spectrumCurveDb()
            : std::array<float, equalizerSpectrumBinCount>{};
        if (!m_effectsUnit) {
            spectrum.fill(-96.0f);
        }

        auto guideVertices = node->guides->geometry()->vertexDataAsPoint2D();
        const std::array<double, 3> guideFrequencies{100.0, 1000.0, 10000.0};
        for (int index = 0; index < 3; ++index) {
            const float x = xFromFrequency(
                guideFrequencies.at(static_cast<std::size_t>(index)), graphWidth);
            guideVertices[index * 2].set(x, 0.0f);
            guideVertices[index * 2 + 1].set(x, graphHeight);
        }
        guideVertices[6].set(0.0f, graphHeight * 0.5f);
        guideVertices[7].set(graphWidth, graphHeight * 0.5f);
        node->guides->geometry()->markVertexDataDirty();
        node->guides->markDirty(QSGNode::DirtyGeometry);

        auto fillVertices = node->spectrumFill->geometry()->vertexDataAsPoint2D();
        auto spectrumVertices = node->spectrumLine->geometry()->vertexDataAsPoint2D();
        for (int index = 0; index < equalizerSpectrumBinCount; ++index) {
            const float x = graphWidth * static_cast<float>(index)
                / static_cast<float>(equalizerSpectrumBinCount - 1);
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

        auto responseVertices = node->responseLine->geometry()->vertexDataAsPoint2D();
        for (int index = 0; index < equalizerResponsePointCount; ++index) {
            const float x = graphWidth * static_cast<float>(index)
                / static_cast<float>(equalizerResponsePointCount - 1);
            responseVertices[index].set(
                x, yFromResponse(response.at(static_cast<std::size_t>(index)),
                                 graphHeight));
        }
        node->responseLine->geometry()->markVertexDataDirty();
        node->responseLine->markDirty(QSGNode::DirtyGeometry);

        setMaterialColor(node->guides, m_guideColor);
        setMaterialColor(node->spectrumFill, m_spectrumColor);
        QColor spectrumLineColor = m_spectrumColor;
        spectrumLineColor.setAlphaF(std::min(1.0, spectrumLineColor.alphaF() * 2.0));
        setMaterialColor(node->spectrumLine, spectrumLineColor);
        setMaterialColor(node->responseLine, m_responseColor);
        return node;
    }

    void EqualizerGraph::geometryChange(const QRectF &newGeometry,
                                        const QRectF &oldGeometry) {
        QQuickItem::geometryChange(newGeometry, oldGeometry);
        if (newGeometry.size() != oldGeometry.size()) {
            update();
        }
    }

}

#include "moc_EqualizerGraph.cpp"
