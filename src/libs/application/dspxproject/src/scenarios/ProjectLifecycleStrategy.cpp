// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ProjectLifecycleStrategy.h"

namespace DspxProject {

    ProjectLifecycleStrategy::ProjectLifecycleStrategy(QObject *parent) : QObject(parent) {
    }

    ProjectLifecycleStrategy::~ProjectLifecycleStrategy() = default;

}

#include "moc_ProjectLifecycleStrategy.cpp"
