// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_P_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_P_H

#include <visualeditor/ParameterEditorRow.h>

#include <QString>

namespace VisualEditor {

    class ParameterEditorRowPrivate {
        Q_DECLARE_PUBLIC(ParameterEditorRow)

    public:
        ParameterEditorRow *q_ptr{};
        ParameterEditorContext *context{};
        ParameterViewModelBinding *editingBinding{};
        ParameterViewModelBinding *referenceBinding{};
        QString editingParameterId;
        QString referenceParameterId;
        QString editingDisplayName;
        QString referenceDisplayName;

        static ParameterEditorRow *create(ParameterEditorContext *context, QObject *parent);
        static ParameterEditorRowPrivate *get(ParameterEditorRow *q);
        static const ParameterEditorRowPrivate *get(const ParameterEditorRow *q);
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_P_H
