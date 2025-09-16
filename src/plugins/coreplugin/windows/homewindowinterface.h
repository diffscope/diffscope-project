#ifndef DIFFSCOPE_COREPLUGIN_HOMEWINDOWINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_HOMEWINDOWINTERFACE_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <coreplugin/actionwindowinterfacebase.h>

namespace QAK {
    class QuickActionContext;
}

namespace Core {

    class HomeWindowInterfacePrivate;

    class CORE_EXPORT HomeWindowInterface : public ActionWindowInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_DECLARE_PRIVATE(HomeWindowInterface)
    public:
        static HomeWindowInterface* instance();

    protected:
        QWindow *createWindow(QObject *parent) const override;

        explicit HomeWindowInterface(QObject *parent = nullptr);
        explicit HomeWindowInterface(HomeWindowInterfacePrivate &d, QObject *parent = nullptr);
        ~HomeWindowInterface() override;

        friend class ExecutiveInterfaceRegistry<HomeWindowInterface>;

    private:
        QScopedPointer<HomeWindowInterfacePrivate> d_ptr;
    };

    class CORE_EXPORT HomeWindowInterfaceRegistry : public ExecutiveInterfaceRegistry<HomeWindowInterface> {
    public:
        static HomeWindowInterfaceRegistry *instance();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_HOMEWINDOWINTERFACE_H
