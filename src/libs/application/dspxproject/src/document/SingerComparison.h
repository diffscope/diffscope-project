// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_SINGERCOMPARISON_H
#define DIFFSCOPE_DSPXPROJECT_SINGERCOMPARISON_H

#include <opendspx/singer.h>

#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    DSPXPROJECT_EXPORT bool singerDefinitionEquals(const opendspx::SingerRef &left, const opendspx::SingerRef &right);

}

#endif // DIFFSCOPE_DSPXPROJECT_SINGERCOMPARISON_H
