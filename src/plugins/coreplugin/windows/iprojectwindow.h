#ifndef DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H

#include <coreplugin/coreglobal.h>

#include <CoreApi/iwindow.h>

namespace QAK {
    class QuickActionContext;
}

namespace Core {

    class IProjectWindowPrivate;

    class CORE_EXPORT IProjectWindow : public IWindow {
        Q_OBJECT
        Q_DECLARE_PRIVATE(IProjectWindow)
    public:
        static IProjectWindow* instance();

        QAK::QuickActionContext *actionContext() const;

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit IProjectWindow(QObject *parent = nullptr);
        explicit IProjectWindow(IProjectWindowPrivate &d, QObject *parent = nullptr);
        ~IProjectWindow() override;

        void nextLoadingState(State nextState) override;

        friend class IExecutiveRegistry<IProjectWindow>;

    private:
        QScopedPointer<IProjectWindowPrivate> d_ptr;
    };

    class CORE_EXPORT IProjectWindowRegistry : public IExecutiveRegistry<IProjectWindow> {
    public:
        static IProjectWindowRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IPROJECTWINDOW_H
