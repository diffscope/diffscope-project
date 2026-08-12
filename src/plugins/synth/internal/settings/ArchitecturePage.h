#ifndef DIFFSCOPE_SYNTH_ARCHITECTUREPAGE_H
#define DIFFSCOPE_SYNTH_ARCHITECTUREPAGE_H

#include <CoreApi/isettingpage.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class ArchitectureExtraModel;

    class ArchitecturePage final : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *configurationModel READ configurationModel CONSTANT)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    public:
        explicit ArchitecturePage(QObject *parent = nullptr);
        ~ArchitecturePage() override;

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

        ArchitectureExtraModel *m_model{};
        QObject *m_widget{};
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_ARCHITECTUREPAGE_H
