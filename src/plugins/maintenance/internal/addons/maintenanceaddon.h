#ifndef DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H
#define DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H

#include <qqmlintegration.h>

#include <QQuickWindow>

#include <CoreApi/windowinterface.h>

namespace Maintenance {

    class MaintenancePlugin;

    class MaintenanceAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit MaintenanceAddOn(QObject *parent = nullptr);
        ~MaintenanceAddOn() override;

        static void setPlugin(MaintenancePlugin *plugin);

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        enum RevealFlag {
            Logs,
            Data,
            Plugins,
            Issue,
            Contribute,
            Community,
            ReleaseLog,
        };
        Q_ENUM(RevealFlag)

        Q_INVOKABLE static void reveal(RevealFlag flag);

        Q_INVOKABLE void generateDiagnosticReport() const;
    };

}

#endif //DIFFSCOPE_MAINTENANCE_MAINTENANCEADDON_H
