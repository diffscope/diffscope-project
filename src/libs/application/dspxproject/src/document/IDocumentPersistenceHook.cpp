// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "IDocumentPersistenceHook.h"

namespace DspxProject {

    IDocumentPersistenceHook::IDocumentPersistenceHook(QObject *parent) : QObject(parent) {}

    IDocumentPersistenceHook::~IDocumentPersistenceHook() = default;

}
