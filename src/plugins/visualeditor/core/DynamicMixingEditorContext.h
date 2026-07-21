#ifndef DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_H
#define DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace dspx {
    class DynamicMixingAnchor;
    class SingingClip;
}

namespace sflow {
    class DynamicMixingAnchorViewModel;
    class DynamicMixingEditorInteractionController;
    class DynamicMixingViewModel;
    class SelectionController;
}

namespace VisualEditor {

    class DynamicMixingEditorContextPrivate;
    class DynamicMixingSelectionController;
    class ProjectViewModelContext;

    class VISUAL_EDITOR_EXPORT DynamicMixingEditorContext : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DynamicMixingEditorContext)
        Q_PROPERTY(dspx::SingingClip *singingClip READ singingClip NOTIFY singingClipChanged)
        Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
        Q_PROPERTY(sflow::DynamicMixingViewModel *dynamicMixingViewModel READ dynamicMixingViewModel CONSTANT)
        Q_PROPERTY(sflow::SelectionController *selectionController READ selectionController CONSTANT)
        Q_PROPERTY(sflow::DynamicMixingEditorInteractionController *interactionController READ interactionController CONSTANT)

    public:
        ~DynamicMixingEditorContext() override;

        dspx::SingingClip *singingClip() const;
        bool isAvailable() const;
        sflow::DynamicMixingViewModel *dynamicMixingViewModel() const;
        sflow::SelectionController *selectionController() const;
        sflow::DynamicMixingEditorInteractionController *interactionController() const;

        Q_INVOKABLE dspx::DynamicMixingAnchor *getDocumentItemFromViewItem(
            sflow::DynamicMixingAnchorViewModel *viewItem) const;
        Q_INVOKABLE sflow::DynamicMixingAnchorViewModel *getViewItemFromDocumentItem(
            dspx::DynamicMixingAnchor *item) const;

        void setSingingClip(dspx::SingingClip *singingClip);

    Q_SIGNALS:
        void singingClipChanged();
        void availableChanged();

    private:
        friend class ProjectViewModelContext;
        friend class DynamicMixingSelectionController;
        explicit DynamicMixingEditorContext(ProjectViewModelContext *projectContext);
        QScopedPointer<DynamicMixingEditorContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_H
