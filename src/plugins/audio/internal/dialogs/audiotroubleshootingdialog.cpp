#include "audiotroubleshootingdialog.h"

#include <cstdlib>

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QCommandLinkButton>
#include <QFormLayout>
#include <QMessageBox>
#include <QPlainTextEdit>

#include <audio/internal/audiosystem.h>
#include <audio/internal/outputsystem.h>
#include <audio/internal/audiooutputsettingshelper.h>

namespace Audio::Internal {

    enum Pages {
        Page_Welcome,
        Page_Test,
        Page_Configure,
        Page_ExternalCheck,
        Page_OK,
        Page_Fail,
    };

    class WelcomePage : public QWizardPage {
        Q_OBJECT
    public:
        explicit WelcomePage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle(tr("<h3>Audio Output Troubleshooting Wizard</h3>"));
            setSubTitle(tr("This wizard will help you diagnose and resolve problems with audio output."));


            auto layout = new QVBoxLayout;
            auto continueButton = new QCommandLinkButton(tr("&Continue"));
            layout->addWidget(continueButton);

            setLayout(layout);

            connect(continueButton, &QAbstractButton::clicked, parent, &QWizard::next);
        }

        int nextId() const override {
            if (AudioSystem::outputSystem()->isReady()) {
                return Page_Test;
            } else {
                return Page_Configure;
            }
        }
    };

    class TestPage : public QWizardPage {
        Q_OBJECT
    public:
        explicit TestPage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle(tr("<h3>Test Audio Device</h3>"));
            setSubTitle(tr("The audio device appears to be working. Please click the \"Test\" button to check whether it can play sound."));
            auto layout = new QVBoxLayout;
            auto testButtonLayout = new QHBoxLayout;
            auto testButton = new QPushButton(tr("&Test"));
            testButtonLayout->addWidget(testButton);
            testButtonLayout->addStretch();
            layout->addLayout(testButtonLayout);
            auto noteLabel = new QLabel(tr(
                "<p>Before testing, please make sure the device you are monitoring is the one you selected, especially if your computer has multiple audio devices (e.g., speakers and headphones), and the device has been properly connected to your computer.</p>"
                "<p>Also please check whether the device is muted or the volume is set to zero.</p>"
            ));
            noteLabel->setWordWrap(true);
            layout->addWidget(noteLabel);
            auto line = new QFrame;
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            layout->addSpacing(16);
            layout->addWidget(line);
            layout->addSpacing(16);
            auto questionLabel = new QLabel(tr("Is any sound played after clicking the \"Test\" button?"));
            questionLabel->setWordWrap(true);
            layout->addWidget(questionLabel);
            auto yesButton = new QCommandLinkButton(
                tr("&Yes"), tr("I can hear the test sound played by the audio device"));
            layout->addWidget(yesButton);
            auto noButton = new QCommandLinkButton(
                tr("&No"), tr("I cannot hear any sound played by the audio device"));
            layout->addWidget(noButton);

            setLayout(layout);

            connect(testButton, &QAbstractButton::clicked, this, [=] {
                parent->helper()->testDevice();
            });

            connect(yesButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_OK;
                parent->next();
            });
            connect(noButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_Configure;
                parent->next();
            });
        }

        int nextId() const override {
            return m_nextId;
        }

    private:
        int m_nextId = Page_OK;
    };

    class ResultOkPage : public QWizardPage {
        Q_OBJECT
    public:
        explicit ResultOkPage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle(tr("<h3>Everything Is OK</h3>"));
            setSubTitle(tr("There are no problems with audio output at the moment."));
            auto layout = new QVBoxLayout;
            auto noteLabel = new QLabel(tr("<p>If you still cannot hear sound when playing back your project, please check for issues within the project itself (e.g., muted tracks or gain set to negative infinity).</p>"));
            noteLabel->setWordWrap(true);
            layout->addWidget(noteLabel);
            setLayout(layout);
        }

        int nextId() const override {
            return -1;
        }
    };

    class ConfigurePage : public QWizardPage {
        Q_OBJECT
    public:
        explicit ConfigurePage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle(tr("<h3>Configure Audio Device</h3>"));
            setSubTitle(tr("Please select a working audio driver and device."));
            auto layout = new QVBoxLayout;
            auto instructionLabel = new QLabel(tr(
                "<p>Please test all audio drivers and devices one by one, and try different sample rates and buffer sizes.</p>"
                "<p>Some devices listed on your computer may be virtual devices and may not output sound to physical hardware.</p>"
            ));
            instructionLabel->setWordWrap(true);
            layout->addWidget(instructionLabel);
            auto deviceLayout = new QFormLayout;
            auto driverComboBox = new QComboBox;
            deviceLayout->addRow(tr("Audio d&river"), driverComboBox);
            auto deviceComboBox = new QComboBox;
            deviceLayout->addRow(tr("Audio &device"), deviceComboBox);
            auto sampleRateComboBox = new QComboBox;
            deviceLayout->addRow(tr("&Sample rate"), sampleRateComboBox);
            auto bufferSizeComboBox = new QComboBox;
            deviceLayout->addRow(tr("&Buffer size"), bufferSizeComboBox);
            layout->addLayout(deviceLayout);
            auto testButtonLayout = new QHBoxLayout;
            auto testButton = new QPushButton(tr("&Test"));
            testButtonLayout->addWidget(testButton);
            testButtonLayout->addStretch();
            layout->addLayout(testButtonLayout);
            auto noteLabel = new QLabel(tr(
                "<p>Before testing, please make sure the device you are monitoring is the one you selected, especially if your computer has multiple audio devices (e.g., speakers and headphones), and the device has been connected to your computer.</p>"
                "<p>Also please check whether the device is muted or the volume is set to zero.</p>"
            ));
            noteLabel->setWordWrap(true);
            layout->addWidget(noteLabel);
            auto line = new QFrame;
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            layout->addSpacing(16);
            layout->addWidget(line);
            layout->addSpacing(16);
            auto questionLabel = new QLabel(tr("Have you selected a working audio device which plays sound after clicking the \"Test\" button?"));
            questionLabel->setWordWrap(true);
            layout->addWidget(questionLabel);
            auto yesButton = new QCommandLinkButton(
                tr("&Yes"), tr("I have selected a working audio device"));
            layout->addWidget(yesButton);
            auto noButton = new QCommandLinkButton(
                tr("&No"), tr("The device is either not available or not able to play sound"));
            layout->addWidget(noButton);
            setLayout(layout);

            connect(testButton, &QAbstractButton::clicked, this, [=] {
                parent->helper()->testDevice();
            });

            connect(yesButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_OK;
                parent->next();
            });
            connect(noButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_ExternalCheck;
                parent->next();
            });

        }

        int nextId() const override {
            return m_nextId;
        }

    private:
        int m_nextId = Page_OK;
    };

    class ExternalCheckPage : public QWizardPage {
        Q_OBJECT
    public:
        explicit ExternalCheckPage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle("<h3>Check Your System Audio Settings</h3>");
            setSubTitle(tr("Please ensure that your audio devices are properly connected to your computer and configured in your system settings."));
            auto layout = new QVBoxLayout;
            auto goToSettingsButtonLayout = new QHBoxLayout;
            auto goToSettingsButton = new QPushButton(
#ifdef Q_OS_MACOS
                tr("&Open System Settings")
#else
                tr("&Open Control Panel")
#endif
            );
            goToSettingsButtonLayout->addWidget(goToSettingsButton);
            goToSettingsButtonLayout->addStretch();
            layout->addLayout(goToSettingsButtonLayout);
            auto hintLabel = new QLabel(tr(
                "<p>Also please check whether other applications can play sound.</p>"
                "<p>If another audio application (e.g., a DAW) can play sound normally, please review that application’s settings. It may be using \"exclusive mode\", which allows only that program to access the audio device.</p>"
            ));
            hintLabel->setWordWrap(true);
            layout->addWidget(hintLabel);
            auto line = new QFrame;
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            layout->addSpacing(16);
            layout->addWidget(line);
            layout->addSpacing(16);
            auto questionLabel = new QLabel(tr("Are audio devices configured properly and able to be played by other applications?"));
            questionLabel->setWordWrap(true);
            layout->addWidget(questionLabel);
            auto yesButton = new QCommandLinkButton(
                tr("&Yes"), tr("The audio device work well on my system. I want to retry configuring it in %1").arg(QApplication::applicationName()));
            layout->addWidget(yesButton);
            auto abortButton = new QCommandLinkButton(
                tr("Not &exactly"), tr("Even though other applications can play sound, I fail to configure the audio device in %1").arg(QApplication::applicationName()));
            layout->addWidget(abortButton);
            auto noButton = new QCommandLinkButton(
                tr("&No"), tr("Other applications also cannot play sound"));
            layout->addWidget(noButton);
            auto idkButton = new QCommandLinkButton(tr("&I'm not sure"));
            layout->addWidget(idkButton);
            setLayout(layout);

            connect(goToSettingsButton, &QAbstractButton::clicked, this, [=] {
#ifdef Q_OS_WINDOWS
                std::system("mmsys.cpl");
                return;
#elif defined(Q_OS_MACOS)
                if (std::system("open 'x-apple.systempreferences:com.apple.preference.sound'") == 0)
                    return;
#endif
                QMessageBox::information(this, {}, tr("%1 cannot open the control panel on your system. Please open it manually").arg(QApplication::applicationName()));
            });

            connect(yesButton, &QAbstractButton::clicked, this, [=] {
                parent->back();
            });
            connect(abortButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_Fail;
                parent->next();
            });
            connect(noButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_Fail;
                parent->next();
            });
            connect(idkButton, &QAbstractButton::clicked, this, [=] {
                m_nextId = Page_Fail;
                parent->next();
            });

        }

        int nextId() const override {
            return m_nextId;
        }

    private:
        int m_nextId = Page_Fail;
    };

    class ResultFailPage : public QWizardPage {
        Q_OBJECT
    public:
        explicit ResultFailPage(AudioTroubleshootingDialog *parent = nullptr) : QWizardPage(parent) {
            setTitle("<h3>Unable to Resolve Audio Output Problem</h3>");
            setSubTitle(tr("Please send us a feedback."));
            auto layout = new QVBoxLayout;
            auto noteLabel = new QLabel(tr("<p>When sending feedback, please include the following information (if possible): <ul>"
                                           "<li>A screenshot of the \"Audio Output\" settings page</li>"
                                           "<li>A screenshot of your system audio settings</li>"
                                           "<li>The audio output configurations you have tried and any error messages</li>"
                                           "<li>The log of application</li></ul></p>"));
            noteLabel->setWordWrap(true);
            layout->addWidget(noteLabel);
            setLayout(layout);
        }

        int nextId() const override {
            return -1;
        }
    };


    AudioTroubleshootingDialog::AudioTroubleshootingDialog(QWidget *parent) : QWizard(parent), m_helper(new AudioOutputSettingsHelper(this)) {
        setWindowTitle(tr("Audio Output Troubleshooting Wizard"));
        setWindowFlag(Qt::WindowContextHelpButtonHint, false);
        setWizardStyle(QWizard::ModernStyle);
        setPage(Page_Welcome, new WelcomePage(this));
        setPage(Page_Test, new TestPage(this));
        setPage(Page_OK, new ResultOkPage(this));
        setPage(Page_Configure, new ConfigurePage(this));
        setPage(Page_ExternalCheck, new ExternalCheckPage(this));
        setPage(Page_Fail, new ResultFailPage(this));

        setButtonLayout({QWizard::Stretch, QWizard::BackButton, QWizard::CancelButton, QWizard::FinishButton});

        resize(800, 800);
    }
    AudioTroubleshootingDialog::~AudioTroubleshootingDialog() = default;
    AudioOutputSettingsHelper *AudioTroubleshootingDialog::helper() const {
        return m_helper;
    }

}

#include "audiotroubleshootingdialog.moc"