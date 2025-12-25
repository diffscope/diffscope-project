#include "CloseSaveCheckAddOn.h"

#include <QDir>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <transactional/TransactionController.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    CloseSaveCheckAddOn::CloseSaveCheckAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    CloseSaveCheckAddOn::~CloseSaveCheckAddOn() = default;

    void CloseSaveCheckAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        windowInterface->addCloseCallback([this] {
            return handleCloseRequest();
        });
    }

    void CloseSaveCheckAddOn::extensionsInitialized() {
    }

    bool CloseSaveCheckAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    bool CloseSaveCheckAddOn::handleCloseRequest() const {
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto projectContext = windowInterface->projectDocumentContext();
        auto document = projectContext->document();
        auto transactionController = document->transactionController();
        auto fileLocker = projectContext->fileLocker();
        if (!fileLocker) {
            return true;
        }

        const bool isClean = transactionController->cleanStep() == transactionController->currentStep();
        const bool notModifiedExternally = !fileLocker->isFileModifiedSinceLastSave();
        const bool isSaved = isClean && notModifiedExternally;

        if (isSaved) {
            return true;
        }

        auto button = SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(),
            windowInterface->window(),
            tr("Do you want to save before closing?"),
            tr("If you choose not to save, a copy of the current project file will be created to help recover your work in case of accidental incorrect operation."),
            SVS::SVSCraft::Yes | SVS::SVSCraft::No | SVS::SVSCraft::Cancel,
            SVS::SVSCraft::Yes
        );

        if (button == SVS::SVSCraft::Yes) {
            return windowInterface->save();
        }
        if (button == SVS::SVSCraft::No) {
            QDir runtimeDataDir(ApplicationInfo::applicationLocation(ApplicationInfo::RuntimeData));
            auto tempPath = runtimeDataDir.filePath(QStringLiteral("latest_unsaved_project.dspx"));
            return projectContext->saveCopy(tempPath);
        }

        return false;
    }

}
