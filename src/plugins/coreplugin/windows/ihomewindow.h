#ifndef DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
#define DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H

#include <coreplugin/coreglobal.h>

#include <CoreApi/iwindow.h>

namespace Core {

    class CORE_EXPORT IHomeWindow : public IWindow {
        Q_OBJECT
    public:
        static IHomeWindow* instance();

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit IHomeWindow(QObject *parent = nullptr);
        ~IHomeWindow() override;

        friend class IExecutiveRegistry<IHomeWindow>;
    };

    class CORE_EXPORT IHomeWindowRegistry : public IExecutiveRegistry<IHomeWindow> {
    public:
        static IHomeWindowRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IHOMEWINDOW_H
