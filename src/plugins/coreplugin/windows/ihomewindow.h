#ifndef DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H

#include <coreplugin/coreglobal.h>

#include <CoreApi/iwindow.h>

namespace QAK {
    class QuickActionContext;
}

namespace Core {

    class IHomeWindowPrivate;

    class CORE_EXPORT IHomeWindow : public IWindow {
        Q_OBJECT
        Q_DECLARE_PRIVATE(IHomeWindow)
        Q_PROPERTY(QAK::ActionContext *actionContext READ actionContext CONSTANT)
    public:
        static IHomeWindow* instance();

        QAK::QuickActionContext *actionContext() const;

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit IHomeWindow(QObject *parent = nullptr);
        explicit IHomeWindow(IHomeWindowPrivate &d, QObject *parent = nullptr);
        ~IHomeWindow() override;

        void nextLoadingState(State nextState) override;

        friend class IExecutiveRegistry<IHomeWindow>;

    private:
        QScopedPointer<IHomeWindowPrivate> d_ptr;
    };

    class CORE_EXPORT IHomeWindowRegistry : public IExecutiveRegistry<IHomeWindow> {
    public:
        static IHomeWindowRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
