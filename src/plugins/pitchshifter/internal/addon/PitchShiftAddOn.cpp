// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PitchShiftAddOn.h"

#include <exception>
#include <memory>

#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QVariant>
#include <QWindow>
#include <QtAlgorithms>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <transactional/TransactionController.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/GlobalAudioContext.h>

#include <pitchshifter/internal/PitchShiftTask.h>

namespace PitchShifter::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPitchShiftAddOn, "diffscope.pitchshifter.addon")

    namespace {

        bool isInDirectoryOrSubdirectory(const QString &filePath, const QDir &directory) {
            const auto relativePath = directory.relativeFilePath(QFileInfo(filePath).absoluteFilePath());
            return relativePath == QStringLiteral(".") || (!relativePath.startsWith(QStringLiteral("..")) && !QDir::isAbsolutePath(relativePath));
        }

    }

    PitchShiftAddOn::PitchShiftAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    PitchShiftAddOn::~PitchShiftAddOn() {
        const auto tasks = m_operations.keys();
        for (auto *task : tasks) {
            task->requestCancel();
        }
        for (auto *task : tasks) {
            task->wait();
        }
        qDeleteAll(m_operations);
        qDeleteAll(tasks);
        m_operations.clear();
    }

    void PitchShiftAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.PitchShifter", "PitchShiftActions");
        if (component.isError()) {
            qFatal() << component.errorString();
            return;
        }
        auto actions = component.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        });
        if (!actions) {
            qFatal() << component.errorString();
            return;
        }
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", windowInterface->actionContext());
    }

    void PitchShiftAddOn::extensionsInitialized() {
    }

    bool PitchShiftAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QString PitchShiftAddOn::sanitizedFileName(const QString &name, const QString &fallback) {
        auto result = name.trimmed();
        result.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
        result.remove(QRegularExpression(QStringLiteral("[ .]+$")));
        if (result.isEmpty()) {
            result = fallback;
        }
        static const QRegularExpression reservedName(
            QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\..*)?$"),
            QRegularExpression::CaseInsensitiveOption
        );
        if (reservedName.match(result).hasMatch()) {
            result.append(u'_');
        }
        return result;
    }

    bool PitchShiftAddOn::execDialog(QObject *dialog) {
        if (!dialog) {
            return false;
        }
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    QObject *PitchShiftAddOn::createAndPositionDialog(QQuickWindow *window, QQmlComponent *component, const QVariantMap &initialProperties) {
        if (!window || !component || component->isError()) {
            return nullptr;
        }
        auto properties = initialProperties;
        properties.insert(QStringLiteral("parent"), QVariant::fromValue(window->contentItem()));
        auto dialog = component->createWithInitialProperties(properties);
        if (!dialog) {
            return nullptr;
        }
        const auto width = dialog->property("width").toDouble();
        const auto height = dialog->property("height").toDouble();
        dialog->setProperty("x", window->width() / 2.0 - width / 2.0);
        if (const auto topMargin = window->property("popupTopMarginHint"); topMargin.isValid()) {
            dialog->setProperty("y", topMargin);
        } else {
            dialog->setProperty("y", window->height() / 2.0 - height / 2.0);
        }
        return dialog;
    }

    dspx::AudioPathInfo PitchShiftAddOn::outputAudioPath(const QString &filePath, const QString &digest) const {
        const QFileInfo fileInfo(filePath);
        dspx::AudioPathInfo path;
        path.absoluteDir = fileInfo.absolutePath();
        path.fileName = fileInfo.fileName();
        path.digest = digest;

        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto documentContext = windowInterface ? windowInterface->projectDocumentContext() : nullptr;
        auto fileLocker = documentContext ? documentContext->fileLocker() : nullptr;
        const auto projectFilePath = fileLocker ? fileLocker->path() : QString();
        if (!projectFilePath.isEmpty()) {
            const auto projectDirectory = QFileInfo(projectFilePath).absoluteDir();
            if (isInDirectoryOrSubdirectory(fileInfo.absoluteFilePath(), projectDirectory)) {
                path.relativeDir = projectDirectory.relativeFilePath(fileInfo.absolutePath());
            }
        }
        return path;
    }

    void PitchShiftAddOn::replaceClipAudio(dspx::AudioClip *clip, const QString &filePath, const QString &digest) {
        // TODO update audio clip time
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto documentContext = windowInterface ? windowInterface->projectDocumentContext() : nullptr;
        auto document = documentContext ? documentContext->document() : nullptr;
        if (!clip || !document) {
            qCCritical(lcPitchShiftAddOn) << "Cannot replace clip audio because the clip or document no longer exists";
            showCritical(tr("Failed to stretch and shift pitch"), tr("The audio was saved, but the original clip is no longer available."));
            return;
        }

        const auto path = outputAudioPath(filePath, digest);
        document->transactionController()->beginScopedTransaction(tr("Stretching and shifting pitch"), [clip, path] {
            clip->setPath(path);
            return true;
        }, [this] {
            qCCritical(lcPitchShiftAddOn) << "Failed to replace audio clip in exclusive transaction";
            showCritical(tr("Failed to stretch and shift pitch"), tr("The audio was saved, but the clip could not be updated."));
        });
    }

    void PitchShiftAddOn::showCritical(const QString &title, const QString &message) const {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        SVS::MessageBox::critical(
            Core::RuntimeInterface::qmlEngine(),
            windowInterface ? windowInterface->window() : nullptr,
            title,
            message
        );
    }

    void PitchShiftAddOn::closeProgressDialog(QWindow *dialog) const {
        if (!dialog) {
            return;
        }
        dialog->setProperty("closeAllowed", true);
        QMetaObject::invokeMethod(dialog, "done");
    }

    void PitchShiftAddOn::shiftPitch(dspx::AudioClip *clip) {
        try {
            auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
            auto quickWindow = windowInterface ? qobject_cast<QQuickWindow *>(windowInterface->window()) : nullptr;
            if (!windowInterface || !quickWindow || !clip) {
                return;
            }

            const QPointer<dspx::AudioClip> guardedClip(clip);
            const auto sourcePath = clip->path();
            auto audioContext = Audio::AudioClipAudioContext::of(clip);
            const auto sourceFilePath = audioContext ? audioContext->realAudioPath() : QString();
            if (sourceFilePath.isEmpty() || !QFileInfo::exists(sourceFilePath)) {
                qCWarning(lcPitchShiftAddOn) << "Resolved source audio file is unavailable" << clip;
                showCritical(tr("Failed to stretch and shift pitch"), tr("The audio file for the selected clip is unavailable."));
                return;
            }

            const QFileInfo sourceFileInfo(sourceFilePath);
            const auto defaultBaseName = sanitizedFileName(clip->name(), sourceFileInfo.completeBaseName());
            auto outputFilePath = QFileDialog::getSaveFileName(
                windowInterface->invisibleCentralWidget(),
                tr("Save Stretched and Pitch-Shifted Audio"),
                sourceFileInfo.dir().filePath(defaultBaseName + QStringLiteral(".wav")),
                tr("WAV Audio (*.wav)")
            );
            if (outputFilePath.isEmpty()) {
                return;
            }
            QFileInfo outputFileInfo(outputFilePath);
            if (outputFileInfo.suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) != 0) {
                auto baseName = outputFileInfo.completeBaseName();
                if (baseName.isEmpty()) {
                    baseName = outputFileInfo.fileName();
                }
                outputFilePath = outputFileInfo.dir().filePath(baseName + QStringLiteral(".wav"));
            }

            QQmlComponent configComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.PitchShifter", "PitchShiftDialog");
            if (configComponent.isError()) {
                qCCritical(lcPitchShiftAddOn) << "Unable to create pitch shift dialog:" << configComponent.errorString();
                showCritical(tr("Failed to stretch and shift pitch"), configComponent.errorString());
                return;
            }
            QScopedPointer<QObject, QScopedPointerDeleteLater> configDialog(createAndPositionDialog(quickWindow, &configComponent, {}));
            if (!configDialog) {
                qCCritical(lcPitchShiftAddOn) << "Unable to create pitch shift dialog:" << configComponent.errorString();
                showCritical(tr("Failed to stretch and shift pitch"), configComponent.errorString());
                return;
            }
            if (!execDialog(configDialog.get())) {
                return;
            }

            PitchShiftConfig config;
            config.outputFilePath = QFileInfo(outputFilePath).absoluteFilePath();
            config.pitchSemitones = configDialog->property("pitch").toDouble();
            config.timeRatio = configDialog->property("stretch").toDouble();
            const auto formantMode = configDialog->property("formantMode").toInt();
            if (formantMode < PitchShiftConfig::ShiftWithPitch || formantMode > PitchShiftConfig::Custom) {
                qCCritical(lcPitchShiftAddOn) << "Pitch shift dialog returned an invalid formant mode" << formantMode;
                showCritical(tr("Failed to stretch and shift pitch"), tr("The selected formant mode is invalid."));
                return;
            }
            config.formantMode = static_cast<PitchShiftConfig::FormantMode>(formantMode);
            config.formantShiftSemitones = configDialog->property("formantShift").toDouble();
            config.linkChannels = configDialog->property("linkChannels").toBool();

            auto inputIo = std::unique_ptr<talcs::AbstractAudioFormatIO>(
                Audio::GlobalAudioContext::formatManager()->getFormatLoad(sourceFilePath, sourcePath.userData, sourcePath.formatEntryClassName));
            if (!inputIo) {
                qCCritical(lcPitchShiftAddOn) << "Unable to create source audio reader for" << QFileInfo(sourceFilePath).fileName();
                showCritical(tr("Failed to stretch and shift pitch"), tr("Unable to open the selected clip's audio file."));
                return;
            }

            auto inputObject = dynamic_cast<QObject *>(inputIo.get());
            auto task = new PitchShiftTask(std::move(inputIo), config);

            QQmlComponent progressComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.PitchShifter", "PitchShiftProgressDialog");
            if (progressComponent.isError()) {
                delete task;
                qFatal() << progressComponent.errorString();
                return;
            }
            auto progressDialog = qobject_cast<QWindow *>(progressComponent.create());
            if (!progressDialog) {
                delete task;
                qFatal() << progressComponent.errorString();
                return;
            }
            progressDialog->setTransientParent(windowInterface->window());
            if (inputObject) {
                inputObject->moveToThread(task);
            }
            m_operations.insert(task, progressDialog);

            QObject::connect(progressDialog, SIGNAL(cancelRequested()), task, SLOT(requestCancel()));
            QObject::connect(progressDialog, SIGNAL(closeRequested()), task, SLOT(requestCancel()));

            connect(task, &PitchShiftTask::progressChanged, this, [progressDialog](int stage, double progress) {
                if (!progressDialog) {
                    return;
                }
                progressDialog->setProperty("stage", stage);
                progressDialog->setProperty("progressValue", progress);
            });

            connect(task, &PitchShiftTask::succeeded, this, [this, guardedClip, outputFilePath = config.outputFilePath, progressDialog](const QString &digest) {
                closeProgressDialog(progressDialog);
                replaceClipAudio(guardedClip, outputFilePath, digest);
            });
            connect(task, &PitchShiftTask::cancelled, this, [this, progressDialog] {
                qCWarning(lcPitchShiftAddOn) << "Pitch shift operation cancelled";
                closeProgressDialog(progressDialog);
            });
            connect(task, &PitchShiftTask::failed, this, [this, progressDialog](const QString &message) {
                closeProgressDialog(progressDialog);
                showCritical(tr("Failed to stretch and shift pitch"), message);
            });
            connect(task, &QThread::finished, this, [this, task, progressDialog] {
                m_operations.remove(task);
                progressDialog->deleteLater();
                task->deleteLater();
            });

            progressDialog->show();
            task->start();
        } catch (const std::exception &exception) {
            const auto message = QString::fromUtf8(exception.what());
            qCCritical(lcPitchShiftAddOn) << "Pitch shift setup failed:" << message;
            showCritical(tr("Failed to stretch and shift pitch"), message);
        } catch (...) {
            qCCritical(lcPitchShiftAddOn) << "Pitch shift setup failed with an unknown exception";
            showCritical(tr("Failed to stretch and shift pitch"), tr("An unknown error occurred while preparing pitch shifting."));
        }
    }

}

#include "moc_PitchShiftAddOn.cpp"
