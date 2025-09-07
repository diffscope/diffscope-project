#ifndef DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <coreplugin/iactionwindowbase.h>

namespace QAK {
    class QuickActionContext;
}

namespace Core {

    class IHomeWindowPrivate;

    class CORE_EXPORT IHomeWindow : public IActionWindowBase {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_DECLARE_PRIVATE(IHomeWindow)
    public:
        static IHomeWindow* instance();

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit IHomeWindow(QObject *parent = nullptr);
        explicit IHomeWindow(IHomeWindowPrivate &d, QObject *parent = nullptr);
        ~IHomeWindow() override;

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
