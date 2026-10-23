// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#include "LibreSVIPPitchMapper.h"

#include <cmath>
#include <memory>
#include <vector>

#include <opendspx/model.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>
#include <opendspx/singingclip.h>

#include <coreplugin/ArchitectureInfo.h>

namespace LibreSVIPFormatConverter::Internal {

    namespace {

        constexpr double maximumPitchValue = 12800.0;

        void mapCurves(std::vector<opendspx::ParamCurveRef> &curves, LibreSVIPPitchMapper::Direction direction) {
            const auto mapValue = [direction](int value) {
                if (direction == LibreSVIPPitchMapper::Direction::FromLibreSVIP) {
                    return Core::ParameterInfo::toDspxModelValue(value / maximumPitchValue);
                }
                return static_cast<int>(std::llround(Core::ParameterInfo::fromDspxModelValue(value) * maximumPitchValue));
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

    void LibreSVIPPitchMapper::mapModel(opendspx::Model &model, Direction direction) {
        for (auto &track : model.content.tracks) {
            for (auto &clip : track.clips) {
                if (!clip || clip->type != opendspx::Clip::Type::Singing) {
                    continue;
                }
                const auto &singingClip = *std::static_pointer_cast<opendspx::SingingClip>(clip);
                if (singingClip.params.find("pitch") == singingClip.params.end()) {
                    continue;
                }
                auto mappedClip = std::make_shared<opendspx::SingingClip>(singingClip);
                auto &pitch = mappedClip->params.at("pitch");
                mapCurves(pitch.original, direction);
                mapCurves(pitch.edited, direction);
                mapCurves(pitch.transform, direction);
                clip = mappedClip;
            }
        }
    }

}
