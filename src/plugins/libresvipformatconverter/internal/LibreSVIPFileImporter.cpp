#include "LibreSVIPFileImporter.h"

#include <sstream>
#include <utility>

#include <QDir>
#include <QFile>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <libresvipformatconverter/internal/LibreSVIPConversionWizard.h>
#include <libresvipformatconverter/internal/LibreSVIPManager.h>
#include <libresvipformatconverter/internal/prebuiltinformats.h>

namespace LibreSVIPFormatConverter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcLibreSVIPFileImporter, "diffscope.libresvipformatconverter.importer")

    static QList<PreBuiltInFormat> catalogFormats(const QList<LibreSVIPPluginInfo> &plugins) {
        QList<PreBuiltInFormat> formats;
        formats.reserve(plugins.size());
        for (const auto &plugin : plugins)
            formats.append({plugin.identifier, plugin.name.isEmpty() ? plugin.identifier : plugin.name, plugin.suffixes});
        return formats;
    }

    LibreSVIPFileImporter::LibreSVIPFileImporter(QObject *parent) : FileConverter(parent) {
        setName(tr("Multiple formats (import via LibreSVIP)"));
        setDescription(tr("Import project files of other editors (such as VOCALOID, OpenUtau, etc.) via LibreSVIP"));
        setMode(Import);
        setHeuristicPriority(Low);
        refreshFormats();
        connect(LibreSVIPManager::instance(), &LibreSVIPManager::catalogChanged,
                this, &LibreSVIPFileImporter::refreshFormats);
    }

    LibreSVIPFileImporter::~LibreSVIPFileImporter() = default;

    bool LibreSVIPFileImporter::runPreExecCheck() {
        return LibreSVIPManager::instance()->runPreExecCheck();
    }

    bool LibreSVIPFileImporter::execImport(const QString &path, opendspx::Model &model, QWindow *window) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(lcLibreSVIPFileImporter) << "Failed to read file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to open file"),
                                      QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        if (file.size() > 512ll * 1024ll * 1024ll) {
            file.close();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window,
                                      tr("LibreSVIP conversion failed"),
                                      tr("The input file is larger than the 512 MiB LibreSVIP conversion limit."));
            return false;
        }
        const QByteArray inputData = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            qCCritical(lcLibreSVIPFileImporter) << "Failed to read file:" << path << file.errorString();
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window, tr("Failed to open file"),
                                      QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), file.errorString()));
            return false;
        }
        file.close();

        LibreSVIPConversionWizard wizard(LibreSVIPConversionWizard::Import, path, inputData, this, window);
        if (!wizard.run())
            return false;
        const auto result = wizard.conversionResult();
        if (result.outputData.isEmpty()) {
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window,
                                      tr("LibreSVIP conversion failed"), tr("LibreSVIP returned no output data."));
            return false;
        }

        opendspx::SerializationErrorList errors;
        std::stringstream stream(result.outputData.constFirst().toStdString(), std::ios::in | std::ios::binary);
        auto convertedModel = opendspx::Serializer::deserialize(stream, errors);
        if (errors.containsFatal() || errors.containsError()) {
            qCCritical(lcLibreSVIPFileImporter) << "LibreSVIP returned invalid DSPX data for" << path;
            SVS::MessageBox::critical(Core::RuntimeInterface::qmlEngine(), window,
                                      tr("Invalid DSPX data"),
                                      tr("The DSPX data returned by LibreSVIP could not be opened."));
            return false;
        }
        model = std::move(convertedModel);
        return true;
    }

    void LibreSVIPFileImporter::refreshFormats() {
        const auto *manager = LibreSVIPManager::instance();
        const auto &catalog = manager->catalog();
        const auto formats = manager->hasCurrentCatalog() ? catalogFormats(catalog.inputs) : preBuiltInInputFormats();
        setFileDialogFilters(preBuiltInFormatFileDialogFilters(formats));
        setHeuristicFilters(preBuiltInFormatHeuristicFilters(formats));
    }

}
