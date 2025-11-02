#ifndef DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLER_H
#define DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLER_H

#include <qqmlintegration.h>

#include <QObject>

#include <coreplugin/coreglobal.h>

namespace Core {

    class CORE_EXPORT EditActionsHandler : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(EditAction enabledActions READ enabledActions NOTIFY enabledActionsChanged)
        Q_PROPERTY(MoveDirection enabledMoveDirections READ enabledMoveDirections NOTIFY enabledMoveDirectionsChanged)
    public:
        explicit EditActionsHandler(QObject *parent = nullptr);
        ~EditActionsHandler() override;

        enum EditActionFlag : quint64 {
            Cut = 0x0001,
            Copy = 0x0002,
            Paste = 0x0004,
            PasteSpecial = 0x0008,
            SelectAll = 0x0010,
            Deselect = 0x0020,
            Delete = 0x0040,
            SelectCurrent = 0x0080,

            SelectUp = 0x0100,
            SelectDown = 0x0200,
            SelectLeft = 0x0400,
            SelectRight = 0x0800,

            MoveCursorUp = 0x1000,
            MoveCursorDown = 0x2000,
            MoveCursorLeft = 0x4000,
            MoveCursorRight = 0x8000,

            ExtendSelectionUp = 0x00010000,
            ExtendSelectionDown = 0x00020000,
            ExtendSelectionLeft = 0x00040000,
            ExtendSelectionRight = 0x00080000,

            ShrinkSelectionUp = 0x00100000,
            ShrinkSelectionDown = 0x00200000,
            ShrinkSelectionLeft = 0x00400000,
            ShrinkSelectionRight = 0x00800000,

            ScrollUp = 0x01000000,
            ScrollDown = 0x02000000,
            ScrollLeft = 0x04000000,
            ScrollRight = 0x08000000,

            PageUp = 0x10000000,
            PageDown = 0x20000000,
            PageLeft = 0x40000000,
            PageRight = 0x80000000,

            ScrollToValueTop = 0x0000000100000000,
            ScrollToValueBottom = 0x0000000200000000,
            ScrollToTimeStart = 0x0000000400000000,
            ScrollToTimeEnd = 0x0000000800000000,

            ScrollToCurrentTime = 0x0000001000000000,
            GoInsideViewRange = 0x0000002000000000,
        };
        Q_ENUM(EditActionFlag)
        Q_DECLARE_FLAGS(EditAction, EditActionFlag)

        virtual EditAction enabledActions() const = 0;
        Q_INVOKABLE virtual bool triggerEditAction(EditAction action) = 0;

        enum MoveDirectionFlag {
            ValueUp = 0x0001,
            ValueDown = 0x0002,
            TimeBackward = 0x0004,
            TimeForward = 0x0008,
        };
        Q_ENUM(MoveDirectionFlag)
        Q_DECLARE_FLAGS(MoveDirection, MoveDirectionFlag)

        virtual MoveDirection enabledMoveDirections() const = 0;
        Q_INVOKABLE virtual bool move(MoveDirection direction, int offset) = 0;

    signals:
        void enabledActionsChanged(EditAction action);
        void enabledMoveDirectionsChanged(MoveDirection direction);
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditActionsHandler::EditAction)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::EditActionsHandler::MoveDirection)

#endif //DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLER_H
