#ifndef DIFFSCOPE_COREPLUGIN_IACTIONWINDOWBASE_H
#define DIFFSCOPE_COREPLUGIN_IACTIONWINDOWBASE_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <CoreApi/iwindow.h>

namespace QAK {
    class QuickActionContext;
}

class QAbstractItemModel;
class QJSValue;

namespace Core {

    class IActionWindowBasePrivate;

    class CORE_EXPORT IActionWindowBase : public IWindow {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAK::QuickActionContext *actionContext READ actionContext CONSTANT)
        Q_PROPERTY(QWidget *invisibleCentralWidget READ invisibleCentralWidget CONSTANT)
        Q_DECLARE_PRIVATE(IActionWindowBase)
    public:
        QAK::QuickActionContext *actionContext() const;
        QWidget *invisibleCentralWidget() const;

        Q_INVOKABLE bool triggerAction(const QString &id, QObject *source = nullptr);

        Q_INVOKABLE int execQuickPick(QAbstractItemModel *model, const QString &placeholderText = {}, int defaultIndex = 0, const QString &initialFilterText = {});
        Q_INVOKABLE QVariant execQuickInput(const QString &placeholderText = {}, const QString &promptText = {}, const QString &initialText = {});
        Q_INVOKABLE QVariant execQuickInput(const QString &placeholderText, const QString &promptText, const QString &initialText, const QJSValue &callback);

    protected:
        explicit IActionWindowBase(QObject *parent = nullptr);
        explicit IActionWindowBase(IActionWindowBasePrivate &d, QObject *parent = nullptr);
        ~IActionWindowBase() override;

        void nextLoadingState(State nextState) override;

    private:
        QScopedPointer<IActionWindowBasePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IACTIONWINDOWBASE_H
