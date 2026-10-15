// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACELAYOUT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACELAYOUT_H

#include <QVariantMap>

namespace Core::Internal {
    class ProjectWindowWorkspaceLayout {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
    public:
        QString name() const;
        void setName(const QString &name);
        bool isValid() const;
        QVariantMap state() const;
        void setState(const QVariantMap &state);
        QVariant toVariant() const;
        static ProjectWindowWorkspaceLayout fromVariant(const QVariant &variant);
        bool operator==(const ProjectWindowWorkspaceLayout &other) const = default;

    private:
        QString m_name;
        QVariantMap m_state;
    };
}

Q_DECLARE_METATYPE(Core::Internal::ProjectWindowWorkspaceLayout)

#endif
