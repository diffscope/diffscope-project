#ifndef DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_H
#define DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

class QQuickItem;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class PlaybackViewModel;
}

namespace VisualEditor {

    class AutoPageScrollingManipulatorPrivate;

    class VISUAL_EDITOR_EXPORT AutoPageScrollingManipulator : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(AutoPageScrollingManipulator)
        Q_PROPERTY(sflow::TimeViewModel *timeViewModel READ timeViewModel WRITE setTimeViewModel NOTIFY timeViewModelChanged)
        Q_PROPERTY(sflow::TimeLayoutViewModel *timeLayoutViewModel READ timeLayoutViewModel WRITE setTimeLayoutViewModel NOTIFY timeLayoutViewModelChanged)
        Q_PROPERTY(sflow::PlaybackViewModel *playbackViewModel READ playbackViewModel WRITE setPlaybackViewModel NOTIFY playbackViewModelChanged)
        Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
        Q_PROPERTY(double viewSize READ viewSize WRITE setViewSize RESET resetViewSize NOTIFY viewSizeChanged)
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    public:
        explicit AutoPageScrollingManipulator(QObject *parent = nullptr);
        ~AutoPageScrollingManipulator() override;

        sflow::TimeViewModel *timeViewModel() const;
        void setTimeViewModel(sflow::TimeViewModel *timeViewModel);

        sflow::TimeLayoutViewModel *timeLayoutViewModel() const;
        void setTimeLayoutViewModel(sflow::TimeLayoutViewModel *timeLayoutViewModel);

        sflow::PlaybackViewModel *playbackViewModel() const;
        void setPlaybackViewModel(sflow::PlaybackViewModel *playbackViewModel);

        QQuickItem *target() const;
        void setTarget(QQuickItem *target);

        double viewSize() const;
        void setViewSize(double viewSize);
        void resetViewSize();

        bool enabled() const;
        void setEnabled(bool enabled);

    Q_SIGNALS:
        void timeViewModelChanged();
        void timeLayoutViewModelChanged();
        void playbackViewModelChanged();
        void targetChanged();
        void viewSizeChanged();
        void enabledChanged();

    private:
        QScopedPointer<AutoPageScrollingManipulatorPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_H