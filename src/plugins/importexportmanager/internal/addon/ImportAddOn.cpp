#include "ImportAddOn.h"

#include <QEventLoop>
#include <QQmlComponent>
#include <QWindow>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>

#include <importexportmanager/FileConverter.h>

namespace ImportExportManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcImportAddOn, "diffscope.importexportmanager.importaddon")

    ImportAddOn::ImportAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ImportAddOn::~ImportAddOn() = default;

    void ImportAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        
        // Load and register ImportAddOnActions QML component
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ImportAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ImportAddOn::extensionsInitialized() {
    }

    bool ImportAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    void ImportAddOn::execImport() const {
        qCInfo(lcImportAddOn) << "Exec import";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ImportExportDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"isExport", QVariant::fromValue(false)},
        });
        std::unique_ptr<QWindow> dialog(qobject_cast<QWindow *>(o));
        QEventLoop eventLoop;
        connect(dialog.get(), SIGNAL(done()), &eventLoop, SLOT(quit()));
        dialog->setTransientParent(windowHandle()->window());
        dialog->show();
        eventLoop.exec();
        if (!dialog->property("accepted").toBool())
            return;
        auto path = dialog->property("path").toString();
        auto converter = dialog->property("selectedConverter").value<FileConverter *>();
        QDspx::Model model;
        if (!converter->execImport(path, model, windowHandle()->window())) {
            // Assume that the converter has already notified the user, so we don't need to show another message box here
            qCCritical(lcImportAddOn) << "Import failed" << path << converter->name() << converter;
            return;
        }
        auto projectDocumentContext = std::make_unique<Core::ProjectDocumentContext>();
        projectDocumentContext->newFile(model, false);
        Core::CoreInterface::createProjectWindow(projectDocumentContext.release());
    }
}

#include "moc_ImportAddOn.cpp"