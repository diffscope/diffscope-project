// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_PARAMETERSPAGE_H
#define DIFFSCOPE_SYNTH_PARAMETERSPAGE_H

#include <QUrl>

#include <CoreApi/isettingpage.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class ParameterConfigurationModel;
    class SynthService;

    class ParametersPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel *configurationModel READ configurationModel CONSTANT)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY messageChanged)
        Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY messageChanged)
    public:
        explicit ParametersPage(SynthService *service = nullptr, QObject *parent = nullptr);
        ~ParametersPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        QAbstractItemModel *configurationModel() const;
        QString errorMessage() const;
        QString statusMessage() const;

        Q_INVOKABLE bool importFromFile(const QUrl &fileUrl);
        Q_INVOKABLE bool exportToFile(const QUrl &fileUrl);

    Q_SIGNALS:
        void messageChanged();

    private:
        bool widgetMatches(const QString &word);
        void setMessages(const QString &error, const QString &status = {});

        SynthService *m_service{};
        ParameterConfigurationModel *m_model{};
        QObject *m_widget{};
        QString m_errorMessage;
        QString m_statusMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_PARAMETERSPAGE_H
