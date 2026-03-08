#ifndef DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H
#define DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core {
    class NotificationMessage;
}

namespace Core::Internal {

    class EditActionsAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit EditActionsAddOn(QObject *parent = nullptr);
        ~EditActionsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        enum ShiftCursorDirection {
            ShiftCursorDirection_Left,
            ShiftCursorDirection_Right,
            ShiftCursorDirection_Up,
            ShiftCursorDirection_Down
        };
        Q_ENUM(ShiftCursorDirection)

        Q_INVOKABLE void shiftCursor(ShiftCursorDirection direction);
        Q_INVOKABLE void selectCurrent();
        Q_INVOKABLE void multipleSelectCurrent();
        Q_INVOKABLE void shiftNotes(int semitone);

    private:
        NotificationMessage *m_pitchOutOfRangeNotification;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITACTIONSADDON_H
