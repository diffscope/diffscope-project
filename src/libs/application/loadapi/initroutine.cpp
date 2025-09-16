#include "initroutine.h"

#include <QtCore/QCoreApplication>

namespace Core {

    static constexpr const char *kSplashPropertyKey = "__loader_splash__";

    int InitRoutine::startMode() {
        return {};
    }

    void InitRoutine::setStartMode(StartMode startMode) {
    }

    InitRoutine::StartEntry InitRoutine::startEntry() {
        return {};
    }

    void InitRoutine::setStartEntry(const StartEntry &startEntry) {
    }

}
