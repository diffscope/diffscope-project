// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterResamplingStrategy.h"

namespace DspxProject {

    ParameterResamplingStrategy::ParameterResamplingStrategy(QObject *parent) : QObject(parent) {
    }

    ParameterResamplingStrategy::~ParameterResamplingStrategy() = default;

}

#include "moc_ParameterResamplingStrategy.cpp"
