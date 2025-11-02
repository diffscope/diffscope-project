#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H

#include <QObject>

#include <coreplugin/coreglobal.h>

namespace sflow {
    class PlaybackViewModel;
}

namespace Core {

    class ProjectTimeline;

    class ProjectViewModelContextPrivate;

    class CORE_EXPORT ProjectViewModelContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectViewModelContext)
        Q_PROPERTY(sflow::PlaybackViewModel *playbackViewModel READ playbackViewModel CONSTANT)

    public:
        explicit ProjectViewModelContext(ProjectTimeline *projectTimeline, QObject *parent = nullptr);
        ~ProjectViewModelContext() override;

        sflow::PlaybackViewModel *playbackViewModel() const;

    private:
        QScopedPointer<ProjectViewModelContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_H
