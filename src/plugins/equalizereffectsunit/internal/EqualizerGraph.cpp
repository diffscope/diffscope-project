// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerGraph.h"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRectangleNode>
#include <QSGRendererInterface>
#include <QSGTransformNode>

#include <SVSCraftQuick/SoftwarePainterNode.h>

#include <equalizereffectsunit/internal/EqualizerEffectsUnit.h>
#include <equalizereffectsunit/internal/EqualizerParameters.h>

namespace EqualizerEffectsUnit::Internal {

    namespace {

        constexpr int guideVertexCount = 8;
        constexpr int spectrumFillVertexCount = equalizerSpectrumBinCount * 2;

        float xFromFrequency(double frequency, float width);
        float yFromResponse(float responseDb, float height);
        float yFromSpectrum(float spectrumDb, float height);

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

        class EqualizerCurvesSoftwareNode : public SVS::SoftwarePainterNode {
        public:
            explicit EqualizerCurvesSoftwareNode(QQuickItem *item) : SoftwarePainterNode(item) {
                setFlag(QSGNode::OwnedByParent);
            }

            void synchronize(const std::array<float, equalizerResponsePointCount> &response,
                             const std::array<float, equalizerSpectrumBinCount> &spectrum,
                             float width,
                             float height,
                             const QColor &responseColor,
                             const QColor &spectrumColor) {
                const QSizeF size(width, height);
                if (m_size != size || m_spectrum != spectrum) {
                    m_spectrum = spectrum;
                    m_spectrumLine = {};
                    m_spectrumFill = {};
                    if (!spectrum.empty()) {
                        m_spectrumFill.moveTo(0, height);
                        for (int index = 0; index < equalizerSpectrumBinCount; ++index) {
                            const float x = width * static_cast<float>(index)
                                / static_cast<float>(equalizerSpectrumBinCount - 1);
                            const float y = yFromSpectrum(spectrum.at(static_cast<std::size_t>(index)), height);
                            if (index == 0) {
                                m_spectrumLine.moveTo(x, y);
                            } else {
                                m_spectrumLine.lineTo(x, y);
                            }
                            m_spectrumFill.lineTo(x, y);
                        }
                        m_spectrumFill.lineTo(width, height);
                        m_spectrumFill.closeSubpath();
                    }
                    markDirty(QSGNode::DirtyGeometry);
                }
                if (m_size != size || m_response != response) {
                    m_response = response;
                    m_responseLine = {};
                    for (int index = 0; index < equalizerResponsePointCount; ++index) {
                        const float x = width * static_cast<float>(index)
                            / static_cast<float>(equalizerResponsePointCount - 1);
                        const QPointF point(x, yFromResponse(response.at(static_cast<std::size_t>(index)), height));
                        if (index == 0) {
                            m_responseLine.moveTo(point);
                        } else {
                            m_responseLine.lineTo(point);
                        }
                    }
                    markDirty(QSGNode::DirtyGeometry);
                }
                m_size = size;
                if (m_responseColor != responseColor || m_spectrumColor != spectrumColor) {
                    m_responseColor = responseColor;
                    m_spectrumColor = spectrumColor;
                    markDirty(QSGNode::DirtyMaterial);
                }
                setBoundingRect(QRectF(QPointF(), size).adjusted(-1, -1, 1, 1));
            }

        protected:
            void paint(QPainter *painter) override {
                painter->setRenderHint(QPainter::Antialiasing);
                painter->fillPath(m_spectrumFill, m_spectrumColor);
                QColor spectrumLineColor = m_spectrumColor;
                spectrumLineColor.setAlphaF(std::min(1.0, spectrumLineColor.alphaF() * 2.0));
                painter->setBrush(Qt::NoBrush);
                QPen spectrumPen(spectrumLineColor, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
                spectrumPen.setCosmetic(true);
                painter->setPen(spectrumPen);
                painter->drawPath(m_spectrumLine);
                QPen responsePen(m_responseColor, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
                responsePen.setCosmetic(true);
                painter->setPen(responsePen);
                painter->drawPath(m_responseLine);
            }

        private:
            std::array<float, equalizerResponsePointCount> m_response{};
            std::array<float, equalizerSpectrumBinCount> m_spectrum{};
            QPainterPath m_spectrumFill;
            QPainterPath m_spectrumLine;
            QPainterPath m_responseLine;
            QSizeF m_size;
            QColor m_responseColor;
            QColor m_spectrumColor;
        };

        class EqualizerGraphSoftwareNode : public QSGTransformNode {
        public:
            EqualizerGraphSoftwareNode(EqualizerGraph *item, QQuickWindow *window) {
                for (auto &guide : guides) {
                    guide = window->createRectangleNode();
                    guide->setFlag(QSGNode::OwnedByParent);
                    appendChildNode(guide);
                }
                curves = new EqualizerCurvesSoftwareNode(item);
                appendChildNode(curves);
            }

            void synchronize(const std::array<float, equalizerResponsePointCount> &response,
                             const std::array<float, equalizerSpectrumBinCount> &spectrum,
                             float width,
                             float height,
                             const QColor &guideColor,
                             const QColor &responseColor,
                             const QColor &spectrumColor) {
                const std::array<double, 3> guideFrequencies{100.0, 1000.0, 10000.0};
                for (int index = 0; index < 3; ++index) {
                    const float x = xFromFrequency(guideFrequencies.at(static_cast<std::size_t>(index)), width);
                    updateGuide(guides.at(static_cast<std::size_t>(index)), QRectF(x - 0.5, 0, 1, height), guideColor);
                }
                updateGuide(guides.at(3), QRectF(0, height * 0.5f - 0.5, width, 1), guideColor);
                curves->synchronize(response, spectrum, width, height, responseColor, spectrumColor);
            }

            std::array<QSGRectangleNode *, 4> guides{};
            EqualizerCurvesSoftwareNode *curves = nullptr;

        private:
            static void updateGuide(QSGRectangleNode *node, const QRectF &rect, const QColor &color) {
                if (node->rect() != rect) {
                    node->setRect(rect);
                }
                if (node->color() != color) {
                    node->setColor(color);
                }
            }
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

        const bool software = window()
            && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        if (software) {
            auto *node = dynamic_cast<EqualizerGraphSoftwareNode *>(oldNode);
            if (!node) {
                delete oldNode;
                node = new EqualizerGraphSoftwareNode(this, window());
            }
            node->synchronize(response,
                              spectrum,
                              graphWidth,
                              graphHeight,
                              m_guideColor,
                              m_responseColor,
                              m_spectrumColor);
            return node;
        }

        auto node = dynamic_cast<EqualizerGraphNode *>(oldNode);
        if (!node) {
            delete oldNode;
            node = new EqualizerGraphNode;
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
