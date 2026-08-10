#ifndef DIFFSCOPE_SYNTH_SERVICESPAGE_H
#define DIFFSCOPE_SYNTH_SERVICESPAGE_H

#include <CoreApi/isettingpage.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class ServiceConfigurationModel;
    class SynthService;

    class ServicesPage final : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *configurationModel READ configurationModel CONSTANT)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    public:
        explicit ServicesPage(SynthService *service = nullptr, QObject *parent = nullptr);
        ~ServicesPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QAbstractItemModel *configurationModel() const;
        QString errorMessage() const;

    Q_SIGNALS:
        void errorMessageChanged();

    private:
        bool widgetMatches(const QString &word);
        void setErrorMessage(const QString &message);

        SynthService *m_service{};
        ServiceConfigurationModel *m_model{};
        QObject *m_widget{};
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_SERVICESPAGE_H
