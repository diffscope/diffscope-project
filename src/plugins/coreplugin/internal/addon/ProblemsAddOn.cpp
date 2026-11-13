// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProblemsAddOn.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/DockingContext.h>

#include <dspxmodelORM/Model.h>

#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/DocumentProblems.h>

namespace Core::Internal {
    ProblemsAddOn::ProblemsAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProblemsAddOn::~ProblemsAddOn() = default;

    void ProblemsAddOn::initialize() {
        auto *window = windowHandle()->cast<ProjectWindowInterface>();
        m_documentProblems = new DocumentProblems(this);
        m_documentProblems->setWindowHandle(window);
        m_documentProblems->setEntity(window->projectDocumentContext()->document()->model());
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProblemsPanel");
            if (component.isError())
                qFatal() << component.errorString();
            auto *object = component.createWithInitialProperties({{"documentProblems", QVariant::fromValue(m_documentProblems)}});
            if (!object)
                qFatal() << component.errorString();
            object->setParent(this);
            window->actionContext()->addAction("org.diffscope.core.panel.problems",
                                               object->property("panelComponent").value<QQmlComponent *>());
        }
        {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProblemsAddOnActions");
            if (component.isError())
                qFatal() << component.errorString();
            auto *object = component.createWithInitialProperties({{"addOn", QVariant::fromValue(this)}});
            if (!object)
                qFatal() << component.errorString();
            object->setParent(this);
            QMetaObject::invokeMethod(object, "registerToContext", window->actionContext());
        }
    }

    void ProblemsAddOn::extensionsInitialized() {
        auto *window = windowHandle()->cast<ProjectWindowInterface>();
        const auto openProblems = [window] {
            if (auto *context = window->getFirstObject<SVS::DockingContext>())
                context->show(context->panel("org.diffscope.core.panel.problems"));
        };
        connect(this, &ProblemsAddOn::showPanelRequested, this, openProblems);
    }

    DocumentProblems *ProblemsAddOn::documentProblems() const {
        return m_documentProblems;
    }
}

#include "moc_ProblemsAddOn.cpp"
