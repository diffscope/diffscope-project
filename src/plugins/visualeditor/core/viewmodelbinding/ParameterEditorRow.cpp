// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ParameterEditorRow.h"
#include "ParameterEditorRow_p.h"

#include <visualeditor/ParameterEditorContext.h>
#include <visualeditor/private/ParameterEditorContext_p.h>

namespace VisualEditor {

    ParameterEditorRowPrivate *ParameterEditorRowPrivate::get(ParameterEditorRow *q) {
        return q->d_func();
    }

    const ParameterEditorRowPrivate *ParameterEditorRowPrivate::get(const ParameterEditorRow *q) {
        return q->d_func();
    }

    ParameterEditorRow *ParameterEditorRowPrivate::create(ParameterEditorContext *context, QObject *parent) {
        return new ParameterEditorRow(context, parent);
    }

    ParameterEditorRow::ParameterEditorRow(ParameterEditorContext *context, QObject *parent)
        : QObject(parent), d_ptr(new ParameterEditorRowPrivate) {
        d_ptr->q_ptr = this;
        d_ptr->context = context;
    }

    ParameterEditorRow::~ParameterEditorRow() = default;

    ParameterViewModelBinding *ParameterEditorRow::editingBinding() const {
        Q_D(const ParameterEditorRow);
        return d->editingBinding;
    }

    ParameterViewModelBinding *ParameterEditorRow::referenceBinding() const {
        Q_D(const ParameterEditorRow);
        return d->referenceBinding;
    }

    ParameterViewModelBinding *ParameterEditorRow::referenceDataBinding() const {
        Q_D(const ParameterEditorRow);
        return d->editingParameterId == d->referenceParameterId ? d->editingBinding : d->referenceBinding;
    }

    QString ParameterEditorRow::editingParameterId() const {
        Q_D(const ParameterEditorRow);
        return d->editingParameterId;
    }

    void ParameterEditorRow::setEditingParameterId(const QString &parameterId) {
        Q_D(ParameterEditorRow);
        ParameterEditorContextPrivate::get(d->context)->setRowEditingParameterId(this, parameterId);
    }

    QString ParameterEditorRow::referenceParameterId() const {
        Q_D(const ParameterEditorRow);
        return d->referenceParameterId;
    }

    void ParameterEditorRow::setReferenceParameterId(const QString &parameterId) {
        Q_D(ParameterEditorRow);
        ParameterEditorContextPrivate::get(d->context)->setRowReferenceParameterId(this, parameterId);
    }

    QString ParameterEditorRow::editingParameterDisplayName() const {
        Q_D(const ParameterEditorRow);
        return d->editingDisplayName;
    }

    QString ParameterEditorRow::referenceParameterDisplayName() const {
        Q_D(const ParameterEditorRow);
        return d->referenceDisplayName;
    }

    void ParameterEditorRow::swapParameters() {
        Q_D(ParameterEditorRow);
        ParameterEditorContextPrivate::get(d->context)->swapRowParameters(this);
    }

}

#include "moc_ParameterEditorRow.cpp"
