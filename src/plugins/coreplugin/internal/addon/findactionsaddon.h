#ifndef DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H
#define DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H

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
        QStringList m_priorityActions;

        void loadSettings();
        void saveSettings() const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_FINDACTIONSADDON_H
