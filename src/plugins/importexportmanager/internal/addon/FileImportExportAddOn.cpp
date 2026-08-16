// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/DspxDocument.h>

#include <importexportmanager/ConverterCollection.h>
#include <importexportmanager/FileConverter.h>

#include <transactional/TransactionController.h>

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
        auto converters = ConverterCollection::fileConverters();

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
        auto converters = ConverterCollection::fileConverters();
        auto filtered = converters | std::views::filter([&](const FileConverter *converter) {
            return converter->mode() == FileConverter::Export;
        });
        return QList(filtered.begin(), filtered.end());
    }

    void FileImportExportAddOn::execImport(FileConverter *converter) const {
        qCInfo(lcFileImportExportAddOn) << "Exec import" << converter << converter->name();
        opendspx::Model model;
        QString path;
        if (!execImportToModel(converter, model, path)) {
            return;
        }
        auto projectDocumentContext = std::make_unique<Core::ProjectDocumentContext>();
        projectDocumentContext->newFile(model, QFileInfo(path).baseName() + ".dspx", false);
        Core::CoreInterface::createProjectWindow(projectDocumentContext.release());
    }

    void FileImportExportAddOn::execImportTracks(FileConverter *converter) const {
        qCInfo(lcFileImportExportAddOn) << "Exec import tracks" << converter << converter->name();
        opendspx::Model importedModel;
        QString path;
        if (!execImportToModel(converter, importedModel, path)) {
            return;
        }

        Q_ASSERT(qobject_cast<Core::ProjectWindowInterface *>(windowHandle()));
        auto projectWindow = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto document = projectWindow->projectDocumentContext()->document();
        auto model = document->model();
        auto trackList = model->tracks();
        const bool replaceTimeline = SVS::MessageBox::question(
                                         Core::RuntimeInterface::qmlEngine(),
                                         windowHandle()->window(),
                                         tr("Replace tempo and time signature"),
                                         tr("Do you want to replace the current tempo and time signature with those from the imported file?")
                                     ) == SVS::SVSCraft::Yes;

        document->transactionController()->beginScopedTransaction(tr("Importing tracks"), [=, &importedModel] {
            if (replaceTimeline) {
                model->tempos()->fromOpenDSPX(importedModel.content.timeline.tempos);
                model->timeSignatures()->fromOpenDSPX(importedModel.content.timeline.timeSignatures);
            }

            const int insertionIndex = trackList->size();
            int offset = 0;
            for (const auto &trackData : importedModel.content.tracks) {
                auto track = model->createTrack();
                track->fromOpenDSPX(trackData);
                if (!trackList->insertItem(insertionIndex + offset, track)) {
                    model->destroyItem(track);
                    return false;
                }
                ++offset;
            }
            return true;
        }, [] {
            qCCritical(lcFileImportExportAddOn) << "Failed to import tracks in exclusive transaction";
        });
    }

    bool FileImportExportAddOn::execImportToModel(FileConverter *converter, opendspx::Model &model, QString &path) const {
        if (!converter->runPreExecCheck()) {
            qCInfo(lcFileImportExportAddOn) << "Import pre-exec check failed";
            return false;
        }
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto defaultDir = settings->value(QStringLiteral("defaultImportExportDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();
        path = QFileDialog::getOpenFileName(nullptr, tr("Import File"), defaultDir, converter->fileDialogFilters().join(";;"));
        if (path.isEmpty()) {
            qCInfo(lcFileImportExportAddOn) << "Import canceled: file not selected";
            return false;
        }
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultImportExportDir"), QFileInfo(path).absolutePath());
        settings->endGroup();
        if (!converter->execImport(path, model, windowHandle()->window())) {
            qCInfo(lcFileImportExportAddOn) << "Import failed or canceled";
            return false;
        }
        return true;
    }

    void FileImportExportAddOn::execExport(FileConverter *converter) const {
        qCInfo(lcFileImportExportAddOn) << "Exec export" << converter << converter->name() << "from window" << windowHandle();
        if (!converter->runPreExecCheck()) {
            qCInfo(lcFileImportExportAddOn) << "Export pre-exec check failed";
            return;
        }
        Q_ASSERT(qobject_cast<Core::ProjectWindowInterface *>(windowHandle()));
        auto projectDocumentContext = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext();
        auto model = projectDocumentContext->document()->model()->toOpenDSPX();
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto defaultDir = settings->value(QStringLiteral("defaultImportExportDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();
        auto path = QFileDialog::getSaveFileName(nullptr, tr("Export File"), defaultDir, converter->fileDialogFilters().join(";;"));
        if (path.isEmpty()) {
            qCInfo(lcFileImportExportAddOn) << "Export canceled: file not selected";
            return;
        }
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultImportExportDir"), QFileInfo(path).absolutePath());
        settings->endGroup();
        if (!converter->execExport(path, model, windowHandle()->window())) {
            qCInfo(lcFileImportExportAddOn) << "Export failed or canceled";
            return;
        }
    }

    bool FileImportExportAddOn::isHomeWindow() const {
        return static_cast<bool>(qobject_cast<Core::HomeWindowInterface *>(windowHandle()));
    }

}

#include "moc_FileImportExportAddOn.cpp"
