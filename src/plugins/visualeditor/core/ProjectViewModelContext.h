#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H

#include <QObject>

#include <visualeditor/visualeditorglobal.h>

namespace sflow {
    class PlaybackViewModel;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    class ProjectViewModelContextPrivate;

    class VISUAL_EDITOR_EXPORT ProjectViewModelContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectViewModelContext)
        Q_PROPERTY(sflow::PlaybackViewModel *playbackViewModel READ playbackViewModel CONSTANT)

    public:
        explicit ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle);
        ~ProjectViewModelContext() override;

        static ProjectViewModelContext *of(const Core::ProjectWindowInterface *windowHandle);

        sflow::PlaybackViewModel *playbackViewModel() const;

    private:
        QScopedPointer<ProjectViewModelContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
