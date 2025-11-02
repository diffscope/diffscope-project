#ifndef DIFFSCOPE_COREPLUGIN_VIEWVISIBILITYADDON_H
#define DIFFSCOPE_COREPLUGIN_VIEWVISIBILITYADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class ViewVisibilityAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit ViewVisibilityAddOn(QObject *parent = nullptr);
        ~ViewVisibilityAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        enum ViewVisibilityOption {
            MenuBar,
            ToolBar,
            LeftSideBar,
            RightSideBar,
            TopSideBar,
            BottomSideBar,
            StatusBar,
        };
        Q_ENUM(ViewVisibilityOption)

        Q_INVOKABLE void toggleVisibility(ViewVisibilityOption option, bool visible, QObject *action = nullptr) const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_VIEWVISIBILITYADDON_H
