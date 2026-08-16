// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_P_H
#define DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_P_H

#include <QPointer>

#include <synth/ProjectSynthesisContext.h>

namespace Synth {

    class ProjectSynthesisContextPrivate {
        Q_DECLARE_PUBLIC(ProjectSynthesisContext)

    public:
        explicit ProjectSynthesisContextPrivate(ProjectSynthesisContext *q) : q_ptr(q) {}
        static ProjectSynthesisContextPrivate *get(ProjectSynthesisContext *context) {
            return context->d_func();
        }

        ProjectSynthesisContext *q_ptr{};
        QPointer<Core::ProjectWindowInterface> windowHandle;
        Internal::SynthesisProjectAddOn *controller{};
    };

}

#endif // DIFFSCOPE_SYNTH_PROJECTSYNTHESISCONTEXT_P_H
