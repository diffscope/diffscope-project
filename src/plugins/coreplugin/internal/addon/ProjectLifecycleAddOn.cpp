// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectLifecycleAddOn.h"

#include <QDebug>
#include <QQmlComponent>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxproject/ProjectLifecycleScenario.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    ProjectLifecycleAddOn::ProjectLifecycleAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    void ProjectLifecycleAddOn::initialize() {
        auto *windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        auto *actionContext = windowInterface->actionContext();
        const auto registerActions = [this, actionContext](const char *typeName, const QVariantMap &properties) {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", typeName);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto *object = component.createWithInitialProperties(properties);
            if (!object) {
                qFatal() << component.errorString();
            }
            object->setParent(this);
            QMetaObject::invokeMethod(object, "registerToContext", actionContext);
        };

        registerActions("GlobalProjectLifecycleActions", {{"addOn", QVariant::fromValue(this)}});
        if (auto *projectWindowInterface = qobject_cast<ProjectWindowInterface *>(windowInterface)) {
            registerActions("ProjectWindowLifecycleActions", {{"windowHandle", QVariant::fromValue(projectWindowInterface)}});
        }
    }

    void ProjectLifecycleAddOn::extensionsInitialized() {
    }

    DspxProject::ProjectDocumentContext *ProjectLifecycleAddOn::projectContextForOpening() const {
        auto *project = qobject_cast<ProjectWindowInterface *>(windowHandle());
        return project && !project->isManaged() ? project->projectDocumentContext() : nullptr;
    }

    void ProjectLifecycleAddOn::newProject() {
        auto *scenario = CoreInterface::projectLifecycleScenario();
        auto *window = windowHandle()->window();
        if (auto *current = projectContextForOpening())
            scenario->newProjectForWindow(current, window);
        else
            scenario->newProject({}, window);
    }

    void ProjectLifecycleAddOn::newProjectFromTemplate() {
        auto *scenario = CoreInterface::projectLifecycleScenario();
        auto *window = windowHandle()->window();
        if (auto *current = projectContextForOpening())
            scenario->newProjectFromTemplateForWindow(current, {}, window);
        else
            scenario->newProjectFromTemplate({}, window);
    }

    void ProjectLifecycleAddOn::openProject() {
        auto *scenario = CoreInterface::projectLifecycleScenario();
        auto *window = windowHandle()->window();
        if (auto *current = projectContextForOpening())
            scenario->openFileForWindow(current, {}, window);
        else
            scenario->openFiles({}, window);
    }

}

#include "moc_ProjectLifecycleAddOn.cpp"
