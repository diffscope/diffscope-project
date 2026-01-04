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
    class TimelineInteractionController;
    class ListViewModel;
    class TrackListInteractionController;
    class TrackViewModel;
}

namespace dspx {
    class Label;
    class Tempo;
    class Track;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    namespace Internal {
        class ProjectAddOn;
    }

    class ProjectViewModelContextAttachedType;
    class ProjectViewModelContextPrivate;

    class VISUAL_EDITOR_EXPORT ProjectViewModelContext : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        QML_ATTACHED(ProjectViewModelContextAttachedType)
        Q_DECLARE_PRIVATE(ProjectViewModelContext)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(sflow::PlaybackViewModel *playbackViewModel READ playbackViewModel CONSTANT)
        Q_PROPERTY(sflow::PointSequenceViewModel *tempoSequenceViewModel READ tempoSequenceViewModel CONSTANT)
        Q_PROPERTY(sflow::SelectionController *tempoSelectionController READ tempoSelectionController CONSTANT)
        Q_PROPERTY(sflow::PointSequenceViewModel *labelSequenceViewModel READ labelSequenceViewModel CONSTANT)
        Q_PROPERTY(sflow::SelectionController *labelSelectionController READ labelSelectionController CONSTANT)
        Q_PROPERTY(sflow::ListViewModel *trackListViewModel READ trackListViewModel CONSTANT)
        Q_PROPERTY(sflow::SelectionController *trackSelectionController READ trackSelectionController CONSTANT)

    public:
        ~ProjectViewModelContext() override;

        static ProjectViewModelContext *of(const Core::ProjectWindowInterface *windowHandle);

        static ProjectViewModelContextAttachedType *qmlAttachedProperties(QObject *object);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::PlaybackViewModel *playbackViewModel() const;
        sflow::PointSequenceViewModel *tempoSequenceViewModel() const;
        sflow::PointSequenceViewModel *labelSequenceViewModel() const;
        sflow::ListViewModel *trackListViewModel() const;

        sflow::SelectionController *tempoSelectionController() const;
        sflow::SelectionController *labelSelectionController() const;
        sflow::SelectionController *trackSelectionController() const;

        Q_INVOKABLE sflow::TimelineInteractionController *createAndBindTimelineInteractionController(QObject *parent = nullptr);
        Q_INVOKABLE sflow::LabelSequenceInteractionController *createAndBindLabelSequenceInteractionControllerOfTempo(QObject *parent = nullptr);
        Q_INVOKABLE sflow::LabelSequenceInteractionController *createAndBindLabelSequenceInteractionControllerOfLabel(QObject *parent = nullptr);
        Q_INVOKABLE sflow::TrackListInteractionController *createAndBindTrackListInteractionController(QObject *parent = nullptr);

        Q_INVOKABLE dspx::Tempo *getTempoDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const;
        Q_INVOKABLE sflow::LabelViewModel *getTempoViewItemFromDocumentItem(dspx::Tempo *item) const;
        Q_INVOKABLE dspx::Label *getLabelDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const;
        Q_INVOKABLE sflow::LabelViewModel *getLabelViewItemFromDocumentItem(dspx::Label *item) const;
        Q_INVOKABLE dspx::Track *getTrackDocumentItemFromViewItem(sflow::TrackViewModel *viewItem) const;
        Q_INVOKABLE sflow::TrackViewModel *getTrackViewItemFromDocumentItem(dspx::Track *item) const;


    private:
        friend class Internal::ProjectAddOn;
        explicit ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<ProjectViewModelContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
