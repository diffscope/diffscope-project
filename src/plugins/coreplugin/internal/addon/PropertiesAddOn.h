// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
#define DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H

#include <CoreApi/windowinterface.h>

class QQmlComponent;

namespace dspx {
    class SelectionModel;
}

namespace Core::Internal {

    class PropertiesAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(dspx::SelectionModel *propertySelectionModel READ propertySelectionModel NOTIFY propertySelectionModelChanged)

    public:
        explicit PropertiesAddOn(QObject *parent = nullptr);
        ~PropertiesAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        dspx::SelectionModel *propertySelectionModel() const;
        Q_INVOKABLE static QList<QQmlComponent *> getComponents(const QString &id) ;

    Q_SIGNALS:
        void propertySelectionModelChanged();

    private:
        dspx::SelectionModel *m_propertySelectionModel{};
        dspx::SelectionModel *m_freeParameterClipSelectionModel{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
