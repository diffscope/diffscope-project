#include "FileImportExportAddOn.h"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QWindow>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <opendspx/model.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>

#include <importexportmanager/ConverterCollection.h>
#include <importexportmanager/FileConverter.h>

namespace ImportExportManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcFileImportExportAddOn, "diffscope.importexportmanager.fileimportexportaddOn")

    FileImportExportAddOn::FileImportExportAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    FileImportExportAddOn::~FileImportExportAddOn() = default;

    void FileImportExportAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ActionWindowInterfaceBase>();
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ImportActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }
        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ImportPanel", this);
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            }, component.creationContext());
            o->setParent(this);
            windowInterface->actionContext()->addAction("org.diffscope.importexportmanager.panel.import", o->property("importPanelComponent").value<QQmlComponent *>());
        }
        if (qobject_cast<Core::ProjectWindowInterface *>(windowInterface)) {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.ImportExportManager", "ProjectImportExportActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(this)},
            });
            o->setParent(this);
            QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
        }
    }

    void FileImportExportAddOn::extensionsInitialized() {
    }

    bool FileImportExportAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QList<FileConverter *> FileImportExportAddOn::importConverters(const QString &path) {
        auto converters = ConverterCollection::instance()->fileConverters();

        auto filtered = converters | std::views::filter([&](const FileConverter *converter) {
            if (converter->mode() != FileConverter::Import) {
                return false;
            }

            if (path.isEmpty()) {
                return true;
            }

            const auto filters = converter->heuristicFilters();
            return !filters.isEmpty() && QDir::match(filters, QFileInfo(path).fileName());
        });

        QList imports(filtered.begin(), filtered.end());

        if (!path.isEmpty()) {
            std::ranges::stable_sort(imports, [](const FileConverter *a, const FileConverter *b) {
                return a->heuristicPriority() < b->heuristicPriority();
            });
        }

        return imports;
    }

    QList<FileConverter *> FileImportExportAddOn::exportConverters() {
        auto converters = ConverterCollection::instance()->fileConverters();
        auto filtered = converters | std::views::filter([&](const FileConverter *converter) {
            return converter->mode() == FileConverter::Export;
        });
        return QList(filtered.begin(), filtered.end());
    }

    void FileImportExportAddOn::execImport(FileConverter *converter) const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto defaultDir = settings->value(QStringLiteral("defaultImportExportDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();
        auto path = QFileDialog::getOpenFileName(nullptr, {}, defaultDir, converter->fileDialogFilters().join(";;"));
        if (path.isEmpty()) {
            return;
        }
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultImportExportDir"), QFileInfo(path).absolutePath());
        settings->endGroup();
        QDspx::Model model;
        converter->execImport(path, model, windowHandle()->window());
        auto projectDocumentContext = std::make_unique<Core::ProjectDocumentContext>();
        projectDocumentContext->newFile(model, false);
        Core::CoreInterface::createProjectWindow(projectDocumentContext.release());
    }

    void FileImportExportAddOn::execExport(FileConverter *converter) {
        // TODO
    }

    bool FileImportExportAddOn::isHomeWindow() const {
        return static_cast<bool>(qobject_cast<Core::HomeWindowInterface *>(windowHandle()));
    }

}

#include "moc_FileImportExportAddOn.cpp"