// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EditItemsStrategy.h"

namespace DspxProject {

    EditItemsStrategy::EditItemsStrategy(QObject *parent) : QObject(parent) {
    }

    EditItemsStrategy::~EditItemsStrategy() = default;

}

#include "moc_EditItemsStrategy.cpp"
