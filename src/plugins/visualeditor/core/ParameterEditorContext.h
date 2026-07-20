#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_H

#include <QAbstractItemModel>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <coreplugin/ArchitectureInfo.h>

#include <visualeditor/visualeditorglobal.h>

namespace dspx {
    class SingingClip;
}

namespace sflow {
    class AnchorParameterViewModel;
    class FreeParameterViewModel;
    class ParameterEditorInteractionController;
    class ParameterRangeSelectionViewModel;
    class SelectionController;
}

namespace VisualEditor {

    class ParameterViewModelBindingPrivate;

    class VISUAL_EDITOR_EXPORT ParameterViewModelBinding : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParameterViewModelBinding)
        Q_PROPERTY(QString parameterId READ parameterId NOTIFY targetChanged)
        Q_PROPERTY(Core::ParameterInfo parameterInfo READ parameterInfo NOTIFY parameterInfoChanged)
        Q_PROPERTY(Core::ParameterInfo transformParameterInfo READ transformParameterInfo CONSTANT)
        Q_PROPERTY(bool registered READ isRegistered NOTIFY targetChanged)
        Q_PROPERTY(bool available READ isAvailable NOTIFY targetChanged)
        Q_PROPERTY(bool parameterExists READ parameterExists NOTIFY parameterChanged)
        Q_PROPERTY(sflow::FreeParameterViewModel *original READ original CONSTANT)
        Q_PROPERTY(sflow::FreeParameterViewModel *freeEdited READ freeEdited CONSTANT)
        Q_PROPERTY(sflow::FreeParameterViewModel *freeTransform READ freeTransform CONSTANT)
        Q_PROPERTY(sflow::AnchorParameterViewModel *anchorEdited READ anchorEdited CONSTANT)
        Q_PROPERTY(sflow::AnchorParameterViewModel *anchorTransform READ anchorTransform CONSTANT)
        Q_PROPERTY(sflow::ParameterRangeSelectionViewModel *freeSelection READ freeSelection CONSTANT)
        Q_PROPERTY(sflow::ParameterRangeSelectionViewModel *transformFreeSelection READ transformFreeSelection CONSTANT)
        Q_PROPERTY(sflow::SelectionController *anchorSelectionController READ anchorSelectionController CONSTANT)
        Q_PROPERTY(sflow::SelectionController *transformAnchorSelectionController READ transformAnchorSelectionController CONSTANT)
        Q_PROPERTY(sflow::ParameterEditorInteractionController *interactionController READ interactionController CONSTANT)
        Q_PROPERTY(sflow::ParameterEditorInteractionController *transformInteractionController READ transformInteractionController CONSTANT)

    public:
        ~ParameterViewModelBinding() override;

        QString parameterId() const;
        Core::ParameterInfo parameterInfo() const;
        Core::ParameterInfo transformParameterInfo() const;
        bool isRegistered() const;
        bool isAvailable() const;
        bool parameterExists() const;

        sflow::FreeParameterViewModel *original() const;
        sflow::FreeParameterViewModel *freeEdited() const;
        sflow::FreeParameterViewModel *freeTransform() const;
        sflow::AnchorParameterViewModel *anchorEdited() const;
        sflow::AnchorParameterViewModel *anchorTransform() const;
        sflow::ParameterRangeSelectionViewModel *freeSelection() const;
        sflow::ParameterRangeSelectionViewModel *transformFreeSelection() const;
        sflow::SelectionController *anchorSelectionController() const;
        sflow::SelectionController *transformAnchorSelectionController() const;
        sflow::ParameterEditorInteractionController *interactionController() const;
        sflow::ParameterEditorInteractionController *transformInteractionController() const;

        Q_INVOKABLE void focusFreeLayer();
        Q_INVOKABLE void focusAnchorLayer();
        Q_INVOKABLE void focusTransformFreeLayer();
        Q_INVOKABLE void focusTransformAnchorLayer();

    Q_SIGNALS:
        void targetChanged();
        void parameterInfoChanged();
        void parameterChanged();

    private:
        friend class ParameterEditorContextPrivate;
        explicit ParameterViewModelBinding(QObject *parent, bool editable);
        QScopedPointer<ParameterViewModelBindingPrivate> d_ptr;
    };

    class ProjectViewModelContext;
    class ParameterEditorContextPrivate;

    class VISUAL_EDITOR_EXPORT ParameterEditorContext : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParameterEditorContext)
        Q_PROPERTY(dspx::SingingClip *singingClip READ singingClip NOTIFY singingClipChanged)
        Q_PROPERTY(QAbstractItemModel *parameterModel READ parameterModel CONSTANT)
        Q_PROPERTY(QString editingParameterId READ editingParameterId WRITE setEditingParameterId NOTIFY editingParameterIdChanged)
        Q_PROPERTY(QString referenceParameterId READ referenceParameterId WRITE setReferenceParameterId NOTIFY referenceParameterIdChanged)
        Q_PROPERTY(QString editingParameterDisplayName READ editingParameterDisplayName NOTIFY editingParameterIdChanged)
        Q_PROPERTY(QString referenceParameterDisplayName READ referenceParameterDisplayName NOTIFY referenceParameterIdChanged)
        Q_PROPERTY(bool transformEditing READ transformEditing WRITE setTransformEditing NOTIFY transformEditingChanged)
        Q_PROPERTY(ParameterViewModelBinding *pitchBinding READ pitchBinding CONSTANT)
        Q_PROPERTY(ParameterViewModelBinding *editingBinding READ editingBinding CONSTANT)
        Q_PROPERTY(ParameterViewModelBinding *referenceBinding READ referenceBinding CONSTANT)
        Q_PROPERTY(ParameterViewModelBinding *referenceDataBinding READ referenceDataBinding NOTIFY referenceDataBindingChanged)

    public:
        ~ParameterEditorContext() override;

        dspx::SingingClip *singingClip() const;
        QAbstractItemModel *parameterModel() const;

        QString editingParameterId() const;
        void setEditingParameterId(const QString &parameterId);
        QString referenceParameterId() const;
        void setReferenceParameterId(const QString &parameterId);
        QString editingParameterDisplayName() const;
        QString referenceParameterDisplayName() const;
        bool transformEditing() const;
        void setTransformEditing(bool transformEditing);

        ParameterViewModelBinding *pitchBinding() const;
        ParameterViewModelBinding *editingBinding() const;
        ParameterViewModelBinding *referenceBinding() const;
        ParameterViewModelBinding *referenceDataBinding() const;

        void setSingingClip(dspx::SingingClip *singingClip);

        Q_INVOKABLE void swapParameters();

    Q_SIGNALS:
        void singingClipChanged();
        void editingParameterIdChanged();
        void referenceParameterIdChanged();
        void referenceDataBindingChanged();
        void transformEditingChanged();

    private:
        friend class ProjectViewModelContext;
        explicit ParameterEditorContext(ProjectViewModelContext *projectContext);
        QScopedPointer<ParameterEditorContextPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_H
