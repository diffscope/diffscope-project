#ifndef UISHELL_ACTIONLAYOUTSEDITORHELPER_P_H
#define UISHELL_ACTIONLAYOUTSEDITORHELPER_P_H

#include <qqmlintegration.h>

#include <QObject>

class QJSValue;

namespace QAK {
    class ActionRegistry;
    class ActionLayoutEntry;
}

namespace UIShell {

    class ActionLayoutsEditorHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QAK::ActionRegistry *actionRegistry READ actionRegistry WRITE setActionRegistry NOTIFY actionRegistryChanged)

    public:
        explicit ActionLayoutsEditorHelper(QObject *parent = nullptr);
        ~ActionLayoutsEditorHelper() override;

        QAK::ActionRegistry *actionRegistry() const;
        void setActionRegistry(QAK::ActionRegistry *actionRegistry);

        enum ActionType { // keep identical to ActionLayoutEntry::Type
            Action,
            Group,
            Menu,
            Separator,
            Stretch,
        };
        Q_ENUM(ActionType)

        Q_INVOKABLE QJSValue getActionDisplayInfo(const QString &id, const QAK::ActionLayoutEntry &entry) const;
        Q_INVOKABLE static QString highlightString(const QString &s, const QString &t, const QColor &c);

    Q_SIGNALS:
        void actionRegistryChanged();

    private:
        QAK::ActionRegistry *m_actionRegistry;
    };

}

#endif //UISHELL_ACTIONLAYOUTSEDITORHELPER_P_H
