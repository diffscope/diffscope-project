#ifndef DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H
#define DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H

#include <CoreApi/windowinterface.h>
#include <qqmlintegration.h>

#include <QQuickWindow>

namespace Maintenance {

    class MaintenancePlugin;

    class MaintenanceAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QQuickWindow *window READ window CONSTANT)
   public:
        explicit MaintenanceAddOn(QObject *parent = nullptr);
        ~MaintenanceAddOn() override;

        static void setPlugin(MaintenancePlugin *plugin);
        static QQuickWindow *window();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE static void execMaintenance();
    };

}

#endif //DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H
