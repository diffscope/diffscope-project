// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFileDialogs.h"
#include "ProjectFileDialogs_p.h"

#include <memory>

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QWindow>

#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/DspxInspectorDialog.h>
#include <coreplugin/ProjectDocumentContext.h>

namespace Core {

    ProjectFileDialogs::ProjectFileDialogs(QWindow *window) : d_ptr(new ProjectFileDialogsPrivate{.window = window}) {
    }

    ProjectFileDialogs::~ProjectFileDialogs() = default;

    QString ProjectFileDialogs::requestPath(const ProjectPathRequest &request) {
        Q_D(ProjectFileDialogs);
        const bool saving = request.purpose == ProjectPathRequest::SaveAs || request.purpose == ProjectPathRequest::SaveCopy;
        const auto directoryKey = saving ? QStringLiteral("defaultSaveDir") : QStringLiteral("defaultOpenDir");
        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.projectFileDialog"));
        const auto defaultDirectory = state->value(directoryKey, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        state->endGroup();

        QFileDialog dialog;
        dialog.setNameFilters({tr("DiffScope Project Exchange Format (*.dspx)"), tr("All Files (*)")});
        dialog.setAcceptMode(saving ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
        dialog.setFileMode(saving ? QFileDialog::AnyFile : QFileDialog::ExistingFile);
        if (saving && !request.currentPath.isEmpty()) {
            const QFileInfo currentFile(request.currentPath);
            dialog.setDirectory(currentFile.absolutePath());
            if (request.purpose == ProjectPathRequest::SaveAs) {
                dialog.selectFile(currentFile.fileName());
            }
        } else {
            dialog.setDirectory(defaultDirectory);
            if (saving && !request.suggestedName.isEmpty()) {
                dialog.selectFile(request.suggestedName);
            }
        }
        if (d->window) {
            dialog.winId();
            dialog.windowHandle()->setTransientParent(d->window);
            dialog.setWindowModality(Qt::WindowModal);
        }

        if (dialog.exec() != QDialog::Accepted) {
            return {};
        }
        const auto path = dialog.selectedFiles().value(0);
        if (!path.isEmpty()) {
            state->beginGroup(QStringLiteral("org.diffscope.core.projectFileDialog"));
            state->setValue(directoryKey, QFileInfo(path).absolutePath());
            state->endGroup();
        }
        return path;
    }

    void ProjectFileDialogsPrivate::reportParseError(const QString &path) {
        QQmlComponent buttonComponent(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "Button");
        if (buttonComponent.isError()) {
            qFatal() << buttonComponent.errorString();
        }
        auto buttonObject = buttonComponent.create();
        if (!buttonObject) {
            qFatal() << buttonComponent.errorString();
        }
        auto button = qobject_cast<QQuickButton *>(buttonObject);
        if (!button) {
            qFatal("Button in SVSCraft.UIComponents is not QQuickButton");
        }
        button->setText(ProjectFileDialogs::tr("Open DSPX Inspector"));
        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QObject> object(component.createWithInitialProperties(
            {{"text", ProjectFileDialogs::tr("Failed to parse file content")},
             {"informativeText", ProjectFileDialogs::tr("%1\n\nYou can check for problems in the file with DSPX Inspector.").arg(QDir::toNativeSeparators(path))},
             {"icon", SVS::SVSCraft::Critical},
             {"transientParent", QVariant::fromValue(window.data())},
             {"content", QVariant::fromValue(button)}}
        ));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto mb = qobject_cast<QQuickWindow *>(object.get());
        if (!mb) {
            qFatal("MessageBoxDialog in SVSCraft.UIComponents is not QQuickWindow");
        }
        button->setParent(mb);
        QObject::connect(button, &QQuickButton::clicked, mb, [path, mb] {
            QTimer::singleShot(0, [path] {
                DspxInspectorDialog dialog;
                dialog.setPath(path);
                dialog.runCheck();
                dialog.exec();
            });
            mb->close();
        });
        SVS::MessageBox::customExec(mb);
    }

    bool ProjectFileDialogs::confirmWarning(const ProjectWarning &warning) {
        Q_D(ProjectFileDialogs);
        auto engine = RuntimeInterface::qmlEngine();
        switch (warning.code) {
            case ProjectWarning::OtherEditor:
                return SVS::MessageBox::question(
                    engine,
                    d->window,
                    tr("File created with another application"),
                    tr("This project file was created with another application (%1). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                        .arg(warning.detail.isEmpty() ? tr("name unknown") : warning.detail)
                ) == SVS::SVSCraft::Yes;
            case ProjectWarning::IncompatibleVersion:
                return SVS::MessageBox::question(
                    engine,
                    d->window,
                    tr("File created with a potentially incompatible %1 version").arg(QApplication::applicationDisplayName()),
                    tr("This project file was created with a newer version or test version of %1 (%2). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                        .arg(QApplication::applicationDisplayName(), warning.detail)
                ) == SVS::SVSCraft::Yes;
            case ProjectWarning::CheckerWarning:
                return SVS::MessageBox::question(
                    engine,
                    d->window,
                    tr("Additional check failed"),
                    tr("%1\n\nThe file can still be opened, but it may cause potential problems.\n\nDo you want to continue opening it?").arg(warning.detail)
                ) == SVS::SVSCraft::Yes;
            case ProjectWarning::SaveIntegrity:
                return SVS::MessageBox::warning(
                    engine,
                    d->window,
                    tr("Potential document integrity issue"),
                    tr("The document integrity check failed while saving the file. The file can still be saved, but it may not be parsed correctly when opened again.\n\nUnder normal circumstances, this problem should not occur. It may be caused by defects in %1 or some plugins.\n\nIf you are currently using Save or overwriting an existing file, it is recommended that you cancel the current operation. Instead, use Save Copy to save the document in another file, and then inspect the file using DSPX Inspector.\n\nDo you want to continue?").arg(QApplication::applicationDisplayName()),
                    SVS::SVSCraft::Yes | SVS::SVSCraft::No
                ) == SVS::SVSCraft::Yes;
        }
        Q_UNREACHABLE();
    }

    ProjectLifecycleStrategy::ExternalChangeChoice ProjectFileDialogs::resolveExternalChange(const QString &) {
        Q_D(ProjectFileDialogs);
        enum { SaveAs = 1, Overwrite };
        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QObject> object(component.createWithInitialProperties({
            {"text", tr("File Modified Externally")},
            {"informativeText", tr("The file has been modified by another program since it was last saved.\n\nDo you want to save as a new file or overwrite it?")},
            {"buttons", QVariantList{
                QVariantMap{{"id", SaveAs}, {"text", tr("Save As...")}, {"role", QDialogButtonBox::AcceptRole}},
                QVariantMap{{"id", Overwrite}, {"text", tr("Overwrite")}, {"role", QDialogButtonBox::DestructiveRole}},
                SVS::SVSCraft::Cancel,
            }},
            {"primaryButton", SaveAs},
            {"icon", SVS::SVSCraft::Warning},
            {"transientParent", QVariant::fromValue(d->window.data())},
        }));
        if (!object) {
            qFatal() << component.errorString();
        }
        auto window = qobject_cast<QQuickWindow *>(object.get());
        if (!window) {
            qFatal("MessageBoxDialog in SVSCraft.UIComponents is not QQuickWindow");
        }
        switch (SVS::MessageBox::customExec(window).toInt()) {
            case SaveAs:
                return ProjectLifecycleStrategy::ExternalChangeChoice::SaveAs;
            case Overwrite:
                return ProjectLifecycleStrategy::ExternalChangeChoice::Overwrite;
            default:
                return ProjectLifecycleStrategy::ExternalChangeChoice::Cancel;
        }
    }

    ProjectLifecycleStrategy::CloseChoice ProjectFileDialogs::confirmClose(const QString &, const QString &) {
        Q_D(ProjectFileDialogs);
        const auto button = SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(),
            d->window,
            tr("Do you want to save before closing?"),
            tr("If you choose not to save, a copy of the current project file will be created to help recover your work in case of accidental incorrect operation."),
            SVS::SVSCraft::Yes | SVS::SVSCraft::No | SVS::SVSCraft::Cancel,
            SVS::SVSCraft::Yes
        );
        switch (button) {
            case SVS::SVSCraft::Yes:
                return ProjectLifecycleStrategy::CloseChoice::Save;
            case SVS::SVSCraft::No:
                return ProjectLifecycleStrategy::CloseChoice::Discard;
            default:
                return ProjectLifecycleStrategy::CloseChoice::Cancel;
        }
    }

    void ProjectFileDialogs::notifyPartialRecovery(const ProjectFileError &) {
        Q_D(ProjectFileDialogs);
        SVS::MessageBox::warning(
            RuntimeInterface::qmlEngine(),
            d->window,
            tr("Complete recovery failed"),
            tr("Complete recovery failed. Partial recovery will be attempted."),
            SVS::SVSCraft::Ok,
            SVS::SVSCraft::Ok
        );
    }

    void ProjectFileDialogs::reportError(const ProjectFileError &error) {
        Q_D(ProjectFileDialogs);
        auto engine = RuntimeInterface::qmlEngine();
        const auto path = QDir::toNativeSeparators(error.path);
        const auto details = path.isEmpty() ? error.detail : (error.detail.isEmpty() ? path : path + QStringLiteral("\n\n") + error.detail);
        switch (error.code) {
            case ProjectFileError::None:
                return;
            case ProjectFileError::Read:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to open file"), details);
                return;
            case ProjectFileError::Parse:
                d->reportParseError(error.path);
                return;
            case ProjectFileError::Load:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to Load Project"), details);
                return;
            case ProjectFileError::Write:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to save file"), details);
                return;
            case ProjectFileError::Restore:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to restore recovery file"), tr("Cannot restore the recovery file."));
                return;
            case ProjectFileError::Busy:
                SVS::MessageBox::warning(
                    engine,
                    d->window,
                    QCoreApplication::translate("Core::ProjectWindowInterface", "Operation Conflict"),
                    QCoreApplication::translate("Core::ProjectWindowInterface", "Please wait until the current operation is complete before proceeding.")
                );
                return;
            case ProjectFileError::InvalidState:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to save file"), tr("The project cannot be saved in its current state."));
                return;
            case ProjectFileError::CopyToCurrentFile:
                SVS::MessageBox::critical(engine, d->window, tr("Failed to save file"), tr("Choose a different path for the project copy."));
                return;
            case ProjectFileError::CheckerWarning:
                SVS::MessageBox::warning(engine, d->window, tr("Additional check failed"), details);
                return;
            case ProjectFileError::IntegrityWarning:
                SVS::MessageBox::warning(engine, d->window, tr("Potential document integrity issue"), details);
                return;
        }
    }

}
