// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace VisualEditor {

    class ParameterEditorContext;
    class ParameterEditorRowPrivate;
    class ParameterViewModelBinding;

    class VISUAL_EDITOR_EXPORT ParameterEditorRow : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParameterEditorRow)
        Q_PROPERTY(ParameterViewModelBinding *editingBinding READ editingBinding CONSTANT)
        Q_PROPERTY(ParameterViewModelBinding *referenceBinding READ referenceBinding CONSTANT)
        Q_PROPERTY(ParameterViewModelBinding *referenceDataBinding READ referenceDataBinding NOTIFY referenceDataBindingChanged)
        Q_PROPERTY(QString editingParameterId READ editingParameterId WRITE setEditingParameterId NOTIFY editingParameterIdChanged)
        Q_PROPERTY(QString referenceParameterId READ referenceParameterId WRITE setReferenceParameterId NOTIFY referenceParameterIdChanged)
        Q_PROPERTY(QString editingParameterDisplayName READ editingParameterDisplayName NOTIFY editingParameterIdChanged)
        Q_PROPERTY(QString referenceParameterDisplayName READ referenceParameterDisplayName NOTIFY referenceParameterIdChanged)

    public:
        ~ParameterEditorRow() override;

        ParameterViewModelBinding *editingBinding() const;
        ParameterViewModelBinding *referenceBinding() const;
        ParameterViewModelBinding *referenceDataBinding() const;
        QString editingParameterId() const;
        void setEditingParameterId(const QString &parameterId);
        QString referenceParameterId() const;
        void setReferenceParameterId(const QString &parameterId);
        QString editingParameterDisplayName() const;
        QString referenceParameterDisplayName() const;

        Q_INVOKABLE void swapParameters();

    Q_SIGNALS:
        void editingParameterIdChanged();
        void referenceParameterIdChanged();
        void referenceDataBindingChanged();

    private:
        explicit ParameterEditorRow(ParameterEditorContext *context, QObject *parent = nullptr);
        QScopedPointer<ParameterEditorRowPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORROW_H
