// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTEDITORCHECKER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTEDITORCHECKER_H

#include <dspxproject/IChecker.h>

namespace Core::Internal {

    class ProjectEditorChecker : public DspxProject::IChecker {
    public:
        using IChecker::IChecker;

        QList<DspxProject::CheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) override;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTEDITORCHECKER_H
