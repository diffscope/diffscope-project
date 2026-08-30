// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ITEMSELECTORADDON_H
#define DIFFSCOPE_COREPLUGIN_ITEMSELECTORADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class ItemSelectorDialog;

    class ItemSelectorAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

    public:
        explicit ItemSelectorAddOn(QObject *parent = nullptr);
        ~ItemSelectorAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE void showItemSelector();

    private:
        ItemSelectorDialog *m_dialog = nullptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ITEMSELECTORADDON_H
