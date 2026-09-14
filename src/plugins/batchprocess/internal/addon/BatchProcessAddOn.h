// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

class QAbstractItemModel;

namespace BatchProcess::Internal {

    class BatchProcessAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAbstractItemModel *actionModel READ actionModel CONSTANT)
    public:
        explicit BatchProcessAddOn(QObject *parent = nullptr);
        ~BatchProcessAddOn() override;

        QAbstractItemModel *actionModel() const;

        Q_INVOKABLE void executeAction(QObject *actionObject);
        Q_INVOKABLE void reloadScripts();

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
        QAbstractItemModel *m_actionModel{};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSADDON_H
