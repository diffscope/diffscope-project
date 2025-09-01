#ifndef DIFFSCOPE_COREPLUGIN_QUICKINPUT_P_H
#define DIFFSCOPE_COREPLUGIN_QUICKINPUT_P_H

#include <coreplugin/quickinput.h>

#include <QPointer>

#include <coreplugin/iprojectwindow.h>

namespace Core {

    class QuickInputPrivate {
        Q_DECLARE_PUBLIC(QuickInput)
    public:
        QuickInput *q_ptr;
        QString placeholderText;
        QString promptText;
        QString text;
        SVS::SVSCraft::ControlType status{SVS::SVSCraft::CT_Normal};
        bool acceptable{true};
        QPointer<IProjectWindow> windowHandle;
        bool visible{false};
        QPointer<QObject> inputPalette;
        
        void connectInputPalette();
        void disconnectInputPalette();
        void syncToInputPalette();
        void clearInputPalette();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_QUICKINPUT_P_H
