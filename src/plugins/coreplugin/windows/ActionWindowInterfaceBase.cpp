#include "ActionWindowInterfaceBase.h"

#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStandardItemModel>
#include <QWidget>

#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ActionHelper.h>
#include <coreplugin/QuickInput.h>
#include <coreplugin/QuickPick.h>

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

    void ActionWindowInterfaceBase::execQuickPick(QQuickMenu *menu) {
        QStandardItemModel model;
        for (int i = 0; i < menu->count(); i++) {
            if (auto action = menu->actionAt(i)) {
                if (!action->isEnabled())
                    continue;
                auto item = new QStandardItem;
                auto text = Internal::ActionHelper::removeMnemonic(action->text());
                item->setData(action->isCheckable() ? tr("Toggle \"%1\"").arg(text) : text, SVS::SVSCraft::CP_TitleRole);
                item->setData(QVariant::fromValue(action), Qt::DisplayRole);
                model.appendRow(item);
            } else if (auto subMenu = menu->menuAt(i)) {
                if (!subMenu->isEnabled())
                    continue;
                auto item = new QStandardItem;
                item->setData(Internal::ActionHelper::removeMnemonic(tr("Open Menu \"%1\"...").arg(subMenu->title())), SVS::SVSCraft::CP_TitleRole);
                item->setData(QVariant::fromValue(subMenu), Qt::DisplayRole);
                model.appendRow(item);
            }
        }
        int index = execQuickPick(&model, Internal::ActionHelper::removeMnemonic(menu->title()));
        if (index >= 0) {
            if (auto action = model.item(index)->data(Qt::DisplayRole).value<QQuickAction *>()) {
                if (!action->isEnabled())
                    return;
                return action->trigger(window()->property("contentItem").value<QObject *>());
            }
            if (auto subMenu = model.item(index)->data(Qt::DisplayRole).value<QQuickMenu *>()) {
                if (!subMenu->isEnabled())
                    return;
                return execQuickPick(subMenu);
            }
        }
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

#include "moc_ActionWindowInterfaceBase.cpp"
