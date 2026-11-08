// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_SINGERSETTINGS_H
#define DIFFSCOPE_COREPLUGIN_SINGERSETTINGS_H

#include <dspxproject/SourcesSelection.h>

#include <coreplugin/coreglobal.h>

namespace Core::SingerSettings {

    CORE_EXPORT DspxProject::SourcesSelection defaultSinger();
    CORE_EXPORT bool setDefaultSinger(const DspxProject::SourcesSelection &sources);
    // Applies the default singer within the caller's active transaction. Returns whether sources changed.
    CORE_EXPORT bool applyDefaultSinger(dspx::SingingClip *clip);

}

#endif // DIFFSCOPE_COREPLUGIN_SINGERSETTINGS_H
