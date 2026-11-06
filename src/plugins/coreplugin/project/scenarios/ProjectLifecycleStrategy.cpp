// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectLifecycleStrategy.h"

namespace Core {

    ProjectLifecycleStrategy::ProjectLifecycleStrategy(QObject *parent) : QObject(parent) {
    }

    ProjectLifecycleStrategy::~ProjectLifecycleStrategy() = default;

}

#include "moc_ProjectLifecycleStrategy.cpp"
