// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_P_H
#define DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_P_H

#include <dspxproject/EditSourcesScenario.h>

#include <QPointer>

namespace DspxProject {

    class EditSourcesScenarioPrivate {
        Q_DECLARE_PUBLIC(EditSourcesScenario)
    public:
        EditSourcesScenario *q_ptr;
        Document *const document;
        EditSourcesStrategy *const strategy;
        const SingerCatalog::SingerRegistry *const singerRegistry;

        void applySources(const SourcesSelection &sources, const QList<QPointer<dspx::SingingClip>> &clips, QQuickWindow *window) const;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITSOURCESSCENARIO_P_H
