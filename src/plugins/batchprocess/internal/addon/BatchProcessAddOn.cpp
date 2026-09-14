// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessAddOn.h"

#include <QAbstractItemModel>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QSortFilterProxyModel>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/ScriptAction.h>
#include <batchprocess/internal/BatchProcessRuntime.h>
#include <batchprocess/private/BatchProcessInterface_p.h>

namespace BatchProcess::Internal {

    namespace {

        class ScriptActionFilterModel : public QSortFilterProxyModel {
        public:
            explicit ScriptActionFilterModel(bool includeProjectActions, QObject *parent = nullptr)
                : QSortFilterProxyModel(parent), m_includeProjectActions(includeProjectActions) {
            }

        protected:
            bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
                const auto model = sourceModel();
                if (!model || sourceParent.isValid()) {
                    return false;
                }

                const auto index = model->index(sourceRow, 0, sourceParent);
                if (!index.data(ScriptActionModel::SeparatorRole).toBool()) {
                    return m_includeProjectActions || !index.data(ScriptActionModel::RequiresProjectRole).toBool();
                }

                const auto acceptsAction = [this, model, &sourceParent](int row) {
                    const auto actionIndex = model->index(row, 0, sourceParent);
                    return !actionIndex.data(ScriptActionModel::SeparatorRole).toBool() &&
                           (m_includeProjectActions || !actionIndex.data(ScriptActionModel::RequiresProjectRole).toBool());
                };
                const auto hasAcceptedAction = [&acceptsAction](int first, int last) {
                    for (auto row = first; row < last; ++row) {
                        if (acceptsAction(row)) {
                            return true;
                        }
                    }
                    return false;
                };

                const auto rowCount = model->rowCount(sourceParent);
                bool hasSourceActionAfter = false;
                for (auto row = sourceRow + 1; row < rowCount; ++row) {
                    if (!model->index(row, 0, sourceParent).data(ScriptActionModel::SeparatorRole).toBool()) {
                        hasSourceActionAfter = true;
                        break;
                    }
                }

                const auto hasAcceptedActionBefore = hasAcceptedAction(0, sourceRow);
                if (!hasSourceActionAfter) {
                    return hasAcceptedActionBefore;
                }
                return hasAcceptedActionBefore && hasAcceptedAction(sourceRow + 1, rowCount);
            }

        private:
            bool m_includeProjectActions{};
        };

    }

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessAddOn, "diffscope.batchprocess.addon")

    BatchProcessAddOn::BatchProcessAddOn(QObject *parent) : Core::WindowInterfaceAddOn(parent) {
    }

    BatchProcessAddOn::~BatchProcessAddOn() = default;

    QAbstractItemModel *BatchProcessAddOn::actionModel() const {
        return m_actionModel;
    }

    void BatchProcessAddOn::executeAction(QObject *actionObject) {
        const auto action = qobject_cast<BatchProcess::ScriptAction *>(actionObject);
        BatchProcessInterface::instance()->executeAction(action, windowHandle()->cast<Core::ActionWindowInterfaceBase>());
    }

    void BatchProcessAddOn::reloadScripts() {
        BatchProcessInterface::instance()->reloadScripts();
    }

    void BatchProcessAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        auto actionModel = new ScriptActionFilterModel(qobject_cast<Core::ProjectWindowInterface *>(windowInterface), this);
        actionModel->setSourceModel(BatchProcessInterface::instance()->d_func()->actionModel);
        m_actionModel = actionModel;
        qCDebug(lcBatchProcessAddOn) << "Installing Batch Process actions" << windowInterface;
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.BatchProcess", "BatchProcessActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        QMetaObject::invokeMethod(object, "registerToContext", windowInterface->actionContext());
    }

    void BatchProcessAddOn::extensionsInitialized() {
    }

    bool BatchProcessAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

}

#include "moc_BatchProcessAddOn.cpp"
