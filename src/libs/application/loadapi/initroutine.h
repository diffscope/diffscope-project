#ifndef DIFFSCOPE_LOADAPI_INITROUTINE_H
#define DIFFSCOPE_LOADAPI_INITROUTINE_H

#include <QtWidgets/QSplashScreen>

namespace Core {

    class InitRoutine {
    public:
        enum StartMode {
            Application,
            VST,
        };
        using StartEntry = std::function<QWidget *()>;

        // static void initializeAppearance(QSettings *settings);

    public:
        static int startMode();
        static void setStartMode(StartMode startMode);

        static StartEntry startEntry();
        static void setStartEntry(const StartEntry &startEntry);
    };

}

#endif // DIFFSCOPE_LOADAPI_INITROUTINE_H