// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "TransactionalStrategy.h"

namespace Core {
    TransactionalStrategy::TransactionalStrategy(QObject *parent) : QObject(parent) {
    }
    TransactionalStrategy::~TransactionalStrategy() = default;
}
