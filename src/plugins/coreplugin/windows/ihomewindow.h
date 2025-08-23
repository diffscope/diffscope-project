#ifndef DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <CoreApi/iwindow.h>

namespace QAK {
    class QuickActionContext;
}

namespace Core {

    class IHomeWindowPrivate;

    class CORE_EXPORT IHomeWindow : public IWindow {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_DECLARE_PRIVATE(IHomeWindow)
    public:
        static IHomeWindow* instance();

        QAK::QuickActionContext *actionContext() const;

        Q_INVOKABLE bool triggerAction(const QString &id, QObject *source = nullptr);

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
