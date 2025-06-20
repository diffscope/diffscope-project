#include "initroutine.h"

#include <QtCore/QCoreApplication>

namespace Core {

    static constexpr const char *kSplashPropertyKey = "__loader_splash__";

    QSplashScreen *InitRoutine::splash() {
        return qApp->property(kSplashPropertyKey).value<QSplashScreen *>();
    }

    void InitRoutine::setSplash(QSplashScreen *splash) {
        qApp->setProperty(kSplashPropertyKey, QVariant::fromValue(splash));
    }

}