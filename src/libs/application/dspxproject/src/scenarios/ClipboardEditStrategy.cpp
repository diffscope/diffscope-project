// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClipboardEditStrategy.h"

namespace DspxProject {

    ClipboardEditStrategy::ClipboardEditStrategy(QObject *parent) : QObject(parent) {
    }

    ClipboardEditStrategy::~ClipboardEditStrategy() = default;

}

#include "moc_ClipboardEditStrategy.cpp"
