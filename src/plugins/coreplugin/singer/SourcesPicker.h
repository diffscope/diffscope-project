// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_SOURCESPICKER_H
#define DIFFSCOPE_COREPLUGIN_SOURCESPICKER_H

#include <optional>

#include <dspxproject/SourcesSelection.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace Core {

    CORE_EXPORT std::optional<DspxProject::SourcesSelection> pickSources(const DspxProject::SourcesSelection &initial, QQuickWindow *window);

}

#endif // DIFFSCOPE_COREPLUGIN_SOURCESPICKER_H
