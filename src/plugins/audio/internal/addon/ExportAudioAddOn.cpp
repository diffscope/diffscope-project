#include "ExportAudioAddOn.h"

#include <memory>

#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QQmlComponent>
#include <QStandardPaths>
#include <QVariant>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/filelocker.h>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/AudioExporter.h>
#include <audio/AudioExporterConfig.h>
#include <audio/internal/AudioExporterPresets.h>

namespace Audio::Internal {
    namespace {
        QString documentsDirectory() {
            const auto locations = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
            return locations.isEmpty() ? QDir::homePath() : locations.first();
        }

        QString projectDirectory(Core::ProjectWindowInterface *windowInterface) {
            const auto projectDocumentContext = windowInterface->projectDocumentContext();
            if (projectDocumentContext->fileLocker()) {
                const QFileInfo fileInfo(projectDocumentContext->fileLocker()->path());
                if (!fileInfo.filePath().isEmpty() && !fileInfo.dir().isRelative()) {
                    return fileInfo.dir().path();
                }
            }
            return documentsDirectory();
        }

        void applyFileType(AudioExporterConfig &config, int index) {
            const QFileInfo fileInfo(config.fileName());
            config.setFileType(static_cast<AudioExporterConfig::FileType>(index));
            config.setFormatOption(0);
            config.setFileName(fileInfo.completeBaseName() + QStringLiteral(".") +
                               AudioExporterConfig::extensionOfType(static_cast<AudioExporterConfig::FileType>(index)));
        }
    }

    ExportAudioAddOn::ExportAudioAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ExportAudioAddOn::~ExportAudioAddOn() = default;

    void ExportAudioAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "ExportAudioAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ExportAudioAddOn::extensionsInitialized() {
    }

    bool ExportAudioAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    ExportAudioAddOn *ExportAudioAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<ExportAudioAddOn>();
    }

    void ExportAudioAddOn::exportAudio() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !windowInterface->window())
            return;

        AudioExporter exporter(windowInterface, this);

        connect(AudioExporterPresets::instance(), &AudioExporterPresets::currentConfigChanged, &exporter, [&exporter] {
            exporter.setConfig(AudioExporterPresets::instance()->currentConfig());
        });
        exporter.setConfig(AudioExporterPresets::instance()->currentConfig());

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioExportDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> dialog(qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
            {"exporter", QVariant::fromValue(&exporter)},
        })));
        if (!dialog) {
            qFatal() << component.errorString();
        }
        dialog->setTransientParent(windowInterface->window());
        dialog->show();

        QEventLoop eventLoop;
        connect(dialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
    }

    QStringList ExportAudioAddOn::formatOptions(int fileType) const {
        return AudioExporterConfig::formatOptionsOfType(static_cast<AudioExporterConfig::FileType>(fileType));
    }

    void ExportAudioAddOn::browseFile() {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        const QStringList filters = {
            tr("WAV (*.wav)"),
            tr("FLAC (*.flac)"),
            tr("Ogg Vorbis (*.ogg)"),
            tr("MP3 (*.mp3)"),
        };
        QString selectedFilter = filters.at(config.fileType());
        const auto path = QFileDialog::getSaveFileName(
            nullptr,
            {},
            QDir(projectDirectory(windowInterface)).absoluteFilePath(config.fileDirectory()),
            filters.join(QStringLiteral(";;")),
            &selectedFilter
        );
        if (path.isEmpty()) {
            return;
        }

        const QFileInfo fileInfo(path);
        const auto templateSuffix = config.mixingOption() == AudioExporterConfig::MO_Mixed
            ? QStringLiteral(".")
            : QStringLiteral("_${trackIndex}_${trackName}.");
        config.setFileName(fileInfo.completeBaseName() + templateSuffix + fileInfo.suffix());
        config.setFileDirectory(fileInfo.dir().canonicalPath());
        applyFileType(config, filters.indexOf(selectedFilter));
        presets->setCurrentConfig(config);
    }

    void ExportAudioAddOn::setMixingOption(int index) {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        config.setMixingOption(static_cast<AudioExporterConfig::MixingOption>(index));

        const QFileInfo fileInfo(config.fileName());
        auto basename = fileInfo.completeBaseName();
        auto suffix = fileInfo.suffix();
        if (index == AudioExporterConfig::MO_Mixed) {
            if (basename.endsWith(QStringLiteral("_${trackIndex}_${trackName}"))) {
                basename = basename.chopped(27);
            }
        } else if (!basename.contains(QStringLiteral("${trackIndex}")) &&
                   !basename.contains(QStringLiteral("${trackName}"))) {
            basename += QStringLiteral("_${trackIndex}_${trackName}");
        }
        if (suffix.isEmpty()) {
            suffix = AudioExporterConfig::extensionOfType(config.fileType());
        }
        config.setFileName(basename + QStringLiteral(".") + suffix);
        presets->setCurrentConfig(config);
    }

    void ExportAudioAddOn::setFileType(int index) {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        applyFileType(config, index);
        presets->setCurrentConfig(config);
    }

}

#include "moc_ExportAudioAddOn.cpp"
