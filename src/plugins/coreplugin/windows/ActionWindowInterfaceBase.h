#ifndef DIFFSCOPE_COREPLUGIN_ACTIONWINDOWINTERFACEBASE_H
#define DIFFSCOPE_COREPLUGIN_ACTIONWINDOWINTERFACEBASE_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/coreglobal.h>

namespace QAK {
    class QuickActionContext;
}

class QAbstractItemModel;
class QJSValue;

namespace Core {

    class ActionWindowInterfaceBasePrivate;

    class CORE_EXPORT ActionWindowInterfaceBase : public WindowInterface {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_PROPERTY(QWidget *invisibleCentralWidget READ invisibleCentralWidget CONSTANT)
        Q_DECLARE_PRIVATE(ActionWindowInterfaceBase)
    public:
        QAK::QuickActionContext *actionContext() const;
        QWidget *invisibleCentralWidget() const;

        Q_INVOKABLE bool triggerAction(const QString &id, QObject *source = nullptr);

        Q_INVOKABLE int execQuickPick(QAbstractItemModel *model, const QString &placeholderText = {}, int defaultIndex = 0, const QString &initialFilterText = {});
        Q_INVOKABLE QVariant execQuickInput(const QString &placeholderText = {}, const QString &promptText = {}, const QString &initialText = {});
        Q_INVOKABLE QVariant execQuickInput(const QString &placeholderText, const QString &promptText, const QString &initialText, const QJSValue &callback);

    protected:
        explicit ActionWindowInterfaceBase(QObject *parent = nullptr);
        explicit ActionWindowInterfaceBase(ActionWindowInterfaceBasePrivate &d, QObject *parent = nullptr);
        ~ActionWindowInterfaceBase() override;

        void nextLoadingState(State nextState) override;

    private:
        QScopedPointer<ActionWindowInterfaceBasePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_ACTIONWINDOWINTERFACEBASE_H
