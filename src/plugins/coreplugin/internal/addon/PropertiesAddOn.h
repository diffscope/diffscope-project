#ifndef DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
#define DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H

#include <CoreApi/windowinterface.h>

class QQmlComponent;

namespace Core::Internal {

    class PropertiesAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit PropertiesAddOn(QObject *parent = nullptr);
        ~PropertiesAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE static QList<QQmlComponent *> getComponents(const QString &id) ;

    private:
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
