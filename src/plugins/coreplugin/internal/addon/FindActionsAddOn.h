// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H
#define DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H

#include <QString>
#include <QStringList>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class FindActionsModel;

    class FindActionsAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit FindActionsAddOn(QObject *parent = nullptr);
        ~FindActionsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE void findActions();

    private:
        FindActionsModel *m_model{};
        QString m_windowStateId;
        QStringList m_priorityActions;

        void loadSettings();
        void saveSettings() const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H
