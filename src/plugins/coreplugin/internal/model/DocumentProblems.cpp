// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DocumentProblems.h"

#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    DocumentProblems::DocumentProblems(QObject *parent) : QObject(parent) {
    }

    Core::ProjectWindowInterface *DocumentProblems::windowHandle() const {
        return m_windowHandle;
    }

    void DocumentProblems::setWindowHandle(Core::ProjectWindowInterface *windowHandle) {
        if (m_windowHandle == windowHandle)
            return;
        if (m_manager)
            disconnect(m_manager, nullptr, this, nullptr);
        m_windowHandle = windowHandle;
        m_manager = windowHandle ? windowHandle->projectDocumentContext()->document()->problemManager() : nullptr;
        if (m_manager) {
            connect(m_manager, &DspxProject::DocumentProblemManager::problemsChanged, this, [this](const auto &handles) {
                if (m_entity && handles.contains(m_entity->handle()))
                    Q_EMIT problemsChanged();
            });
            connect(m_manager, &DspxProject::DocumentProblemManager::operationsChanged, this, &DocumentProblems::operationsChanged);
        }
        Q_EMIT windowHandleChanged();
        Q_EMIT problemsChanged();
        Q_EMIT operationsChanged();
    }

    QObject *DocumentProblems::entity() const {
        return m_entity;
    }

    void DocumentProblems::setEntity(QObject *object) {
        auto *entity = qobject_cast<dspx::EntityObject *>(object);
        if (m_entity == entity)
            return;
        disconnect(m_entityDestroyed);
        m_entity = entity;
        if (entity) {
            m_entityDestroyed = connect(entity, &QObject::destroyed, this, [this] {
                Q_EMIT entityChanged();
                Q_EMIT problemsChanged();
            });
        }
        Q_EMIT entityChanged();
        Q_EMIT problemsChanged();
    }

    QVariantList DocumentProblems::problems() const {
        QVariantList result;
        if (m_manager && m_entity) {
            for (const auto &problem : m_manager->problems(m_entity)) {
                result.append(QVariantMap{
                    {QStringLiteral("id"), QString::number(static_cast<quint64>(problem.id))},
                    {QStringLiteral("entity"), QVariant::fromValue(static_cast<QObject *>(problem.info.entity.data()))},
                    {QStringLiteral("severity"), static_cast<int>(problem.info.severity)},
                    {QStringLiteral("message"), problem.info.message},
                });
            }
        }
        return result;
    }

    int DocumentProblems::errorCount() const {
        return count(DspxProject::DocumentProblemSeverity::Error);
    }

    int DocumentProblems::warningCount() const {
        return count(DspxProject::DocumentProblemSeverity::Warning);
    }

    int DocumentProblems::count(DspxProject::DocumentProblemSeverity severity) const {
        if (!m_manager || !m_entity)
            return 0;
        DspxProject::DocumentProblemFilter filter;
        filter.severities = DspxProject::DocumentProblemSeverities(severity);
        return m_manager->problems(m_entity, filter).size();
    }

    QVariantList DocumentProblems::actions(const QString &problemId) const {
        QVariantList result;
        if (m_manager) {
            for (const auto &action : m_manager->actions(static_cast<DspxProject::DocumentProblemId>(problemId.toULongLong()))) {
                result.append(QVariantMap{
                    {QStringLiteral("id"), QString::number(static_cast<quint64>(action.id))},
                    {QStringLiteral("text"), action.text},
                });
            }
        }
        return result;
    }

    void DocumentProblems::triggerAction(const QString &problemId, const QString &actionId) {
        if (m_manager)
            m_manager->triggerAction(static_cast<DspxProject::DocumentProblemId>(problemId.toULongLong()),
                                     static_cast<DspxProject::DocumentProblemOperationId>(actionId.toULongLong()));
    }

    void DocumentProblems::navigate(const QString &problemId) {
        if (m_manager)
            m_manager->navigate(static_cast<DspxProject::DocumentProblemId>(problemId.toULongLong()));
    }

}

#include "moc_DocumentProblems.cpp"
