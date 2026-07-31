#include "LibreSVIPFileExporter.h"

#include <sstream>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSaveFile>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <libresvipformatconverter/internal/LibreSVIPConversionWizard.h>
#include <libresvipformatconverter/internal/LibreSVIPManager.h>
#include <libresvipformatconverter/internal/prebuiltinformats.h>

namespace LibreSVIPFormatConverter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcLibreSVIPFileExporter, "diffscope.libresvipformatconverter.exporter")

    static QList<PreBuiltInFormat> catalogFormats(const QList<LibreSVIPPluginInfo> &plugins) {
        QList<PreBuiltInFormat> formats;
        formats.reserve(plugins.size());
        for (const auto &plugin : plugins)
            formats.append({plugin.identifier, plugin.name.isEmpty() ? plugin.identifier : plugin.name, plugin.suffixes});
        return formats;
    }

    LibreSVIPFileExporter::LibreSVIPFileExporter(QObject *parent) : FileConverter(parent) {
        setName(tr("Multiple formats (export via LibreSVIP)"));
        setDescription(tr("Export as project files of other editors (such as VOCALOID, OpenUtau, etc.) via LibreSVIP"));
        setMode(Export);
        setHeuristicPriority(Low);
        refreshFormats();
        connect(LibreSVIPManager::instance(), &LibreSVIPManager::catalogChanged,
                this, &LibreSVIPFileExporter::refreshFormats);
    }

    LibreSVIPFileExporter::~LibreSVIPFileExporter() = default;

    bool LibreSVIPFileExporter::runPreExecCheck() {
        return LibreSVIPManager::instance()->runPreExecCheck();
    }

    bool LibreSVIPFileExporter::execExport(const QString &path, const opendspx::Model &model, QWindow *window) {
        opendspx::SerializationErrorList serializationErrors;
        std::stringstream stream(std::ios::out | std::ios::binary);
        opendspx::Serializer::serialize(stream, model, serializationErrors, opendspx::Serializer::CheckError, true);
        if (serializationErrors.containsFatal() || serializationErrors.containsError()) {
            qCCritical(lcLibreSVIPFileExporter) << "Failed to serialize DSPX data for LibreSVIP";
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window,
                                      tr("LibreSVIP conversion failed"),
                                      tr("The current project could not be serialized as DSPX."));
            return false;
        }

        const QByteArray inputData = QByteArray::fromStdString(stream.str());
        LibreSVIPConversionWizard wizard(LibreSVIPConversionWizard::Export, path, inputData, this, window);
        if (!wizard.run())
            return false;
        const auto result = wizard.conversionResult();
        if (result.outputData.isEmpty()) {
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window,
                                      tr("LibreSVIP conversion failed"), tr("LibreSVIP returned no output data."));
            return false;
        }

        QString outputPath = path;
        const auto plugin = wizard.selectedExternalPlugin();
        if (!plugin.suffixes.isEmpty() && !plugin.suffixes.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive)) {
            const QFileInfo fileInfo(path);
            outputPath = fileInfo.dir().filePath(fileInfo.completeBaseName() + QLatin1Char('.') + plugin.suffixes.constFirst());
        }
        QSaveFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCCritical(lcLibreSVIPFileExporter) << "Failed to write file:" << outputPath << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to save file"),
                                      QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(outputPath), file.errorString()));
            return false;
        }
        if (file.write(result.outputData.constFirst()) != result.outputData.constFirst().size() || !file.commit()) {
            qCCritical(lcLibreSVIPFileExporter) << "Failed to commit file:" << outputPath << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to save file"),
                                      QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(outputPath), file.errorString()));
            return false;
        }
        return true;
    }

    void LibreSVIPFileExporter::refreshFormats() {
        const auto *manager = LibreSVIPManager::instance();
        const auto &catalog = manager->catalog();
        const auto formats = manager->hasCurrentCatalog() ? catalogFormats(catalog.outputs) : preBuiltInOutputFormats();
        setFileDialogFilters(preBuiltInFormatFileDialogFilters(formats));
        setHeuristicFilters(preBuiltInFormatHeuristicFilters(formats));
    }

}
