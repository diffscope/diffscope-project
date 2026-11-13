// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUAL_EDITOR_DOCUMENTPROBLEMS_H
#define DIFFSCOPE_VISUAL_EDITOR_DOCUMENTPROBLEMS_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <qqmlintegration.h>

#include <dspxmodelORM/EntityObject.h>

#include <dspxproject/DocumentProblemManager.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor::Internal {

    class DocumentProblems : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle WRITE setWindowHandle NOTIFY windowHandleChanged)
        Q_PROPERTY(QObject *entity READ entity WRITE setEntity NOTIFY entityChanged)
        Q_PROPERTY(QVariantList problems READ problems NOTIFY problemsChanged)
    public:
        enum Severity {
            Error = static_cast<int>(DspxProject::DocumentProblemSeverity::Error),
            Warning = static_cast<int>(DspxProject::DocumentProblemSeverity::Warning),
            Notice = static_cast<int>(DspxProject::DocumentProblemSeverity::Notice),
        };
        Q_ENUM(Severity)

        explicit DocumentProblems(QObject *parent = nullptr);

        Core::ProjectWindowInterface *windowHandle() const;
        void setWindowHandle(Core::ProjectWindowInterface *windowHandle);
        QObject *entity() const;
        void setEntity(QObject *object);
        QVariantList problems() const;

        Q_INVOKABLE QVariantList actions(const QString &problemId) const;
        Q_INVOKABLE void triggerAction(const QString &problemId, const QString &actionId);
        Q_INVOKABLE void navigate(const QString &problemId);

    Q_SIGNALS:
        void windowHandleChanged();
        void entityChanged();
        void problemsChanged();
        void operationsChanged();

    private:
        QPointer<Core::ProjectWindowInterface> m_windowHandle;
        QPointer<dspx::EntityObject> m_entity;
        QPointer<DspxProject::DocumentProblemManager> m_manager;
        QMetaObject::Connection m_entityDestroyed;
    };

}

Q_MOC_INCLUDE(<coreplugin/ProjectWindowInterface.h>)

#endif // DIFFSCOPE_VISUAL_EDITOR_DOCUMENTPROBLEMS_H
