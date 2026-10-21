// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DsEditorLiteParameterMapper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include <opendspx/model.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>
#include <opendspx/singingclip.h>

#include <coreplugin/ArchitectureInfo.h>

namespace ImportExportManager::Internal {

    namespace {

        enum class Mapping {
            Linear,
            Decibel,
            Tension,
        };

        struct ParameterMapping {
            std::string_view id;
            int minimum;
            int maximum;
            Mapping mapping{Mapping::Linear};

            double toNormalized(int value) const {
                const double boundedValue = std::clamp(value, minimum, maximum);
                if (mapping == Mapping::Decibel) {
                    return 1.0 - std::pow(-boundedValue / 96000.0, 1.0 / 0.8);
                }
                if (mapping == Mapping::Tension) {
                    const double magnitude = 1.0 - std::pow(1.0 - std::abs(boundedValue) / 10000.0, 1.0 / 0.7);
                    return 0.5 + std::copysign(magnitude / 2.0, boundedValue);
                }
                return (boundedValue - minimum) / (maximum - minimum);
            }

            int fromNormalized(double value) const {
                if (mapping == Mapping::Decibel) {
                    return static_cast<int>(std::llround(-96000.0 * std::pow(1.0 - value, 0.8)));
                }
                if (mapping == Mapping::Tension) {
                    const double base = value * 2.0 - 1.0;
                    const double magnitude = (1.0 - std::pow(1.0 - std::abs(base), 0.7)) * 10000.0;
                    return static_cast<int>(std::llround(std::copysign(magnitude, base)));
                }
                return static_cast<int>(std::llround(value * (maximum - minimum) + minimum));
            }
        };

        constexpr std::array parameterMappings{
            ParameterMapping{"pitch", 0, 12700},
            ParameterMapping{"expressiveness", 0, 1000},
            ParameterMapping{"energy", -96000, 0, Mapping::Decibel},
            ParameterMapping{"breathiness", -96000, 0, Mapping::Decibel},
            ParameterMapping{"voicing", -96000, 0, Mapping::Decibel},
            ParameterMapping{"tension", -10000, 10000, Mapping::Tension},
            ParameterMapping{"mouthOpening", 0, 1000},
            ParameterMapping{"gender", -1000, 1000},
            ParameterMapping{"velocity", -1000, 1000},
            ParameterMapping{"toneShift", -1200, 1200},
        };

        const ParameterMapping *findMapping(std::string_view parameterId) {
            const auto it = std::ranges::find(parameterMappings, parameterId, &ParameterMapping::id);
            return it == parameterMappings.end() ? nullptr : &*it;
        }

        void mapCurves(std::vector<opendspx::ParamCurveRef> &curves, const ParameterMapping &mapping, DsEditorLiteParameterMapper::Direction direction) {
            const auto mapValue = [&mapping, direction](int value) {
                if (direction == DsEditorLiteParameterMapper::Direction::FromDsEditorLite) {
                    return Core::ParameterInfo::toDspxModelValue(mapping.toNormalized(value));
                }
                return mapping.fromNormalized(Core::ParameterInfo::fromDspxModelValue(value));
            };
            for (auto &curve : curves) {
                if (!curve) {
                    continue;
                }
                switch (curve->type) {
                    case opendspx::ParamCurve::Free: {
                        auto mappedCurve = std::make_shared<opendspx::ParamCurveFree>(*std::static_pointer_cast<opendspx::ParamCurveFree>(curve));
                        for (auto &value : mappedCurve->values) {
                            value = mapValue(value);
                        }
                        curve = mappedCurve;
                        break;
                    }
                    case opendspx::ParamCurve::Anchor: {
                        auto mappedCurve = std::make_shared<opendspx::ParamCurveAnchor>(*std::static_pointer_cast<opendspx::ParamCurveAnchor>(curve));
                        for (auto &node : mappedCurve->nodes) {
                            node.y = mapValue(node.y);
                        }
                        curve = mappedCurve;
                        break;
                    }
                }
            }
        }

    }

    int DsEditorLiteParameterMapper::fromDsEditorLite(std::string_view parameterId, int value) {
        const auto mapping = findMapping(parameterId);
        return mapping ? Core::ParameterInfo::toDspxModelValue(mapping->toNormalized(value)) : value;
    }

    int DsEditorLiteParameterMapper::toDsEditorLite(std::string_view parameterId, int value) {
        const auto mapping = findMapping(parameterId);
        return mapping ? mapping->fromNormalized(Core::ParameterInfo::fromDspxModelValue(value)) : value;
    }

    void DsEditorLiteParameterMapper::mapModel(opendspx::Model &model, Direction direction) {
        for (auto &track : model.content.tracks) {
            for (auto &clip : track.clips) {
                if (!clip || clip->type != opendspx::Clip::Type::Singing) {
                    continue;
                }
                auto mappedClip = std::make_shared<opendspx::SingingClip>(*std::static_pointer_cast<opendspx::SingingClip>(clip));
                for (auto &[parameterId, parameter] : mappedClip->params) {
                    const auto mapping = findMapping(parameterId);
                    if (!mapping) {
                        continue;
                    }
                    mapCurves(parameter.original, *mapping, direction);
                    mapCurves(parameter.edited, *mapping, direction);
                    mapCurves(parameter.transform, *mapping, direction);
                }
                clip = mappedClip;
            }
        }
    }

}
