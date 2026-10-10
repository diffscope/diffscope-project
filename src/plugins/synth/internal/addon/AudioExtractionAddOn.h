// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_AUDIOEXTRACTIONADDON_H
#define DIFFSCOPE_SYNTH_AUDIOEXTRACTIONADDON_H

#include <memory>

#include <QPointer>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <dspxmodelORM/AudioClip.h>
#include <synth/internal/AudioExtractionTask.h>

namespace Synth::Internal {

    class AudioExtractionAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit AudioExtractionAddOn(QObject *parent = nullptr);
        ~AudioExtractionAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;

        Q_INVOKABLE void extractNotes(dspx::AudioClip *clip);
        Q_INVOKABLE void separateAudio(dspx::AudioClip *clip);
        Q_INVOKABLE QString chooseDirectory(const QString &directory);
        Q_INVOKABLE QString normalizedPath(const QString &path) const;
        Q_INVOKABLE void start(int extractorIndex, const QString &directory);
        Q_INVOKABLE void closeDialog();

    private:
        struct Operation;
        void open(dspx::AudioClip *clip, AudioExtractionTask::Type type);
        void finish();
        void showError(const QString &message);
        bool sourceAvailable() const;

        std::unique_ptr<Operation> m_operation;
        QPointer<QObject> m_dialog;
        QPointer<AudioExtractionTask> m_task;
    };

}

#endif // DIFFSCOPE_SYNTH_AUDIOEXTRACTIONADDON_H
