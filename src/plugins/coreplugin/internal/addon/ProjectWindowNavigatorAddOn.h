#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWNAVIGATORADDON_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWNAVIGATORADDON_H

#include <CoreApi/windowinterface.h>

class QStandardItemModel;

namespace Core {
    class ProjectWindowInterface;
}

namespace Core::Internal {

    class ProjectWindowNavigatorAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(QList<ProjectWindowInterface *> projectWindows READ projectWindows NOTIFY projectWindowsChanged)
    public:
        explicit ProjectWindowNavigatorAddOn(QObject *parent = nullptr);
        ~ProjectWindowNavigatorAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QList<ProjectWindowInterface *> projectWindows() const;

        Q_INVOKABLE static void raiseWindow(ProjectWindowInterface *windowInterface);
        Q_INVOKABLE void navigateToWindow(int step) const;

    Q_SIGNALS:
        void projectWindowsChanged();

    private:
        void updateProjectWindows();

        QList<ProjectWindowInterface *> m_projectWindows;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWNAVIGATORADDON_H
