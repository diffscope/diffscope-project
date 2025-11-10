#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace sflow {
    class PlaybackViewModel;
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

    public:
        ~ProjectViewModelContext() override;

        static ProjectViewModelContext *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::PlaybackViewModel *playbackViewModel() const;

    private:
        friend class Internal::ProjectAddOn;
        explicit ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<ProjectViewModelContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
