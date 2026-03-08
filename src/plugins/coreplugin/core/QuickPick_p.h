#ifndef DIFFSCOPE_COREPLUGIN_QUICKPICK_P_H
#define DIFFSCOPE_COREPLUGIN_QUICKPICK_P_H

#include <QPointer>

#include <CoreApi/windowinterface.h>

#include <coreplugin/QuickPick.h>

namespace Core {

    class QuickPickPrivate {
        Q_DECLARE_PUBLIC(QuickPick)
    public:
        QuickPick *q_ptr;
        QAbstractItemModel *model{};
        QString filterText;
        QString placeholderText;
        int currentIndex{-1};
        QPointer<WindowInterface> windowHandle;
        bool visible{false};
        QPointer<QObject> commandPalette;

        void connectCommandPalette();
        void disconnectCommandPalette();
        void syncToCommandPalette();
        void clearCommandPalette();
    };

}

#endif //DIFFSCOPE_COREPLUGIN_QUICKPICK_P_H
