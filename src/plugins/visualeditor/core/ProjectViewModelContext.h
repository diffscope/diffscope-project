#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace sflow {
    class PlaybackViewModel;
    class PointSequenceViewModel;
    class LabelSequenceInteractionController;
    class LabelViewModel;
    class SelectionController;
}

namespace dspx {
    class Tempo;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    namespace Internal {
        class ProjectAddOn;
    }

    class ProjectViewModelContextPrivate;

    class VISUAL_EDITOR_EXPORT ProjectViewModelContext : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ProjectViewModelContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(sflow::PlaybackViewModel *playbackViewModel READ playbackViewModel CONSTANT)
        Q_PROPERTY(sflow::PointSequenceViewModel *tempoSequenceViewModel READ tempoSequenceViewModel CONSTANT)
        Q_PROPERTY(sflow::SelectionController *tempoSelectionController READ tempoSelectionController CONSTANT)

    public:
        ~ProjectViewModelContext() override;

        static ProjectViewModelContext *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::PlaybackViewModel *playbackViewModel() const;
        sflow::PointSequenceViewModel *tempoSequenceViewModel() const;

        sflow::SelectionController *tempoSelectionController() const;

        Q_INVOKABLE sflow::LabelSequenceInteractionController *createAndBindLabelSequenceInteractionControllerOfTempo(QObject *parent = nullptr);

        Q_INVOKABLE dspx::Tempo *getTempoDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const;
        Q_INVOKABLE sflow::LabelViewModel *getTempoViewItemFromDocumentItem(dspx::Tempo *item) const;


    private:
        friend class Internal::ProjectAddOn;
        explicit ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<ProjectViewModelContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
