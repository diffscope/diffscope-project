#ifndef DIFFSCOPE_AUDIO_AUDIOTROUBLESHOOTINGDIALOG_H
#define DIFFSCOPE_AUDIO_AUDIOTROUBLESHOOTINGDIALOG_H

#include <QWizard>
#include <qqmlintegration.h>

namespace Audio::Internal {

    class AudioOutputSettingsHelper;

    class AudioTroubleshootingDialog : public QWizard {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit AudioTroubleshootingDialog(QWidget *parent = nullptr);
        ~AudioTroubleshootingDialog() override;

        AudioOutputSettingsHelper *helper() const;

    private:
        AudioOutputSettingsHelper *m_helper;
    };

} // Audio

#endif //DIFFSCOPE_AUDIO_AUDIOTROUBLESHOOTINGDIALOG_H
