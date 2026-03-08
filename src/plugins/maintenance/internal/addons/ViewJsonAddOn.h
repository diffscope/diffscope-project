#ifndef DIFFSCOPE_MAINTENANCE_VIEWJSONADDON_H
#define DIFFSCOPE_MAINTENANCE_VIEWJSONADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Maintenance {

    class ViewJsonAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit ViewJsonAddOn(QObject *parent = nullptr);
        ~ViewJsonAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE void generateJsonFileAndOpen();

    private:
        void generateSessionId();
        QByteArray serializeJson() const;

        QString m_sessionId;
    };

}

#endif //DIFFSCOPE_MAINTENANCE_VIEWJSONADDON_H
