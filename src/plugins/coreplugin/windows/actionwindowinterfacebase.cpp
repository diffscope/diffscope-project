#include "actionwindowinterfacebase.h"

#include <QQmlComponent>
#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlEngine>
#include <QWidget>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/quickpick.h>
#include <coreplugin/internal/actionhelper.h>
#include <coreplugin/quickinput.h>

namespace Core {

    class ActionWindowInterfaceBasePrivate {
        Q_DECLARE_PUBLIC(ActionWindowInterfaceBase)
    public:
        ActionWindowInterfaceBase *q_ptr;
        QAK::QuickActionContext *actionContext;
        
        void init() {
            Q_Q(ActionWindowInterfaceBase);
            actionContext = new QAK::QuickActionContext(q);
            initActionContext();
            CoreInterface::actionRegistry()->addContext(actionContext);
        }

        void initActionContext() {
            Q_Q(ActionWindowInterfaceBase);
            actionContext->setMenuComponent(new QQmlComponent(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "Menu", q));
            actionContext->setSeparatorComponent(new QQmlComponent(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            actionContext->setStretchComponent(new QQmlComponent(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            {
                QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "GlobalActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.create();
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
        }
    };

    QAK::QuickActionContext *ActionWindowInterfaceBase::actionContext() const {
        Q_D(const ActionWindowInterfaceBase);
        return d->actionContext;
    }

    QWidget *ActionWindowInterfaceBase::invisibleCentralWidget() const {
        return window()->property("invisibleCentralWidget").value<QWidget *>();
    }

    bool ActionWindowInterfaceBase::triggerAction(const QString &id, QObject *source) {
        Q_D(ActionWindowInterfaceBase);
        return Internal::ActionHelper::triggerAction(d->actionContext, id, source);
    }

    int ActionWindowInterfaceBase::execQuickPick(QAbstractItemModel *model, const QString &placeholderText, int defaultIndex, const QString &initialFilterText) {
        QuickPick quickPick;
        quickPick.setWindowHandle(this);
        quickPick.setModel(model);
        quickPick.setPlaceholderText(placeholderText);
        quickPick.setFilterText(initialFilterText);
        quickPick.setCurrentIndex(defaultIndex);
        return quickPick.exec();
    }

    QVariant ActionWindowInterfaceBase::execQuickInput(const QString &placeholderText, const QString &promptText, const QString &initialText) {
        QuickInput quickInput;
        quickInput.setWindowHandle(this);
        quickInput.setPlaceholderText(placeholderText);
        quickInput.setPromptText(promptText);
        quickInput.setText(initialText);
        return quickInput.exec();
    }

    QVariant ActionWindowInterfaceBase::execQuickInput(const QString &placeholderText, const QString &promptText, const QString &initialText, const QJSValue &callback) {
        if (!callback.isCallable()) {
            auto engine = qmlEngine(this);
            engine->throwError(QJSValue::TypeError, "callback is not callable");
            return {};
        }
        QuickInput quickInput;
        quickInput.setWindowHandle(this);
        quickInput.setPlaceholderText(placeholderText);
        quickInput.setPromptText(promptText);
        quickInput.setText(initialText);
        auto handleCallbackReturn = [&quickInput](const QJSValue &ret) {
            if (ret.isBool()) {
                quickInput.setAcceptable(ret.toBool());
            } else {
                if (ret.hasProperty("acceptable")) {
                    quickInput.setAcceptable(ret.property("acceptable").toBool());
                }
                if (ret.hasProperty("placeholderText")) {
                    quickInput.setPlaceholderText(ret.property("placeholderText").toString());
                }
                if (ret.hasProperty("promptText")) {
                    quickInput.setPromptText(ret.property("promptText").toString());
                }
                if (ret.hasProperty("status")) {
                    quickInput.setStatus(static_cast<SVS::SVSCraft::ControlType>(ret.property("status").toInt()));
                }
            }
        };
        connect(&quickInput, &QuickInput::textChanged, this, [&] {
            handleCallbackReturn(callback.call({quickInput.text(), false}));
        });
        connect(&quickInput, &QuickInput::attemptingAcceptButFailed, this, [&] {
            handleCallbackReturn(callback.call({quickInput.text(), true}));
        });
        handleCallbackReturn(callback.call({quickInput.text(), false}));
        return quickInput.exec();
    }

    ActionWindowInterfaceBase::ActionWindowInterfaceBase(QObject *parent) : ActionWindowInterfaceBase(*new ActionWindowInterfaceBasePrivate, parent) {
    }

    ActionWindowInterfaceBase::ActionWindowInterfaceBase(ActionWindowInterfaceBasePrivate &d, QObject *parent) : WindowInterface(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }

    ActionWindowInterfaceBase::~ActionWindowInterfaceBase() = default;

    void ActionWindowInterfaceBase::nextLoadingState(State nextState) {
        Q_D(ActionWindowInterfaceBase);
        if (nextState == Initialized) {
            d->actionContext->updateElement(QAK::AE_Layouts);
        }
    }

}

#include "moc_actionwindowinterfacebase.cpp"
