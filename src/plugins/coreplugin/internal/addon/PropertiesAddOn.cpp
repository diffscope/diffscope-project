// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "PropertiesAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/FreeParameterSelectionModel.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {
    PropertiesAddOn::PropertiesAddOn(QObject *parent) {
    }
    PropertiesAddOn::~PropertiesAddOn() = default;
    void PropertiesAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto document = windowInterface->projectDocumentContext()->document();
        auto freeSelection = document->freeParameterSelectionModel();
        // Property mapping must not replace the document's active selection context.
        m_freeParameterClipSelectionModel = new dspx::SelectionModel(document->model(), this);
        const auto updatePropertySelectionModel = [this, document, freeSelection] {
            auto singingClip = freeSelection->isActive() ? freeSelection->singingClip() : nullptr;
            if (singingClip) {
                if (!m_freeParameterClipSelectionModel->isItemSelected(singingClip)) {
                    m_freeParameterClipSelectionModel->select(singingClip, dspx::SelectionModel::ClearPreviousSelection | dspx::SelectionModel::Select);
                }
            } else {
                m_freeParameterClipSelectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection);
            }
            auto selectionModel = singingClip ? m_freeParameterClipSelectionModel : document->selectionModel();
            if (m_propertySelectionModel != selectionModel) {
                m_propertySelectionModel = selectionModel;
                Q_EMIT propertySelectionModelChanged();
            }
        };
        connect(freeSelection, &FreeParameterSelectionModel::contextChanged, this, updatePropertySelectionModel);
        connect(freeSelection, &FreeParameterSelectionModel::activeChanged, this, updatePropertySelectionModel);
        updatePropertySelectionModel();
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "PropertiesPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            },
            RuntimeInterface::qmlEngine()->rootContext());
            if (!o) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.core.panel.properties", o->property("propertiesPanelComponent").value<QQmlComponent *>());
        }
    }
    void PropertiesAddOn::extensionsInitialized() {
    }
    bool PropertiesAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    dspx::SelectionModel *PropertiesAddOn::propertySelectionModel() const {
        return m_propertySelectionModel;
    }
    QList<QQmlComponent *> PropertiesAddOn::getComponents(const QString &id) {
        auto list = RuntimeInterface::instance()->getObjects("org.diffscope.core.propertyeditor." + id);
        QList<QQmlComponent *> result;
        for (auto obj : list) {
            if (auto component = qobject_cast<QQmlComponent *>(obj)) {
                result.append(component);
            }
        }
        return result;
    }
}

#include "moc_PropertiesAddOn.cpp"
