// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspacePanelHistory.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>

namespace Core::Internal {
    WorkspacePanelHistory::WorkspacePanelHistory(QObject *parent) : QObject(parent) {
        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        m_hasPreviousSnapshot = state->value(QStringLiteral("hasPreviousSnapshot"), false).toBool();
        const auto previous = state->value(QStringLiteral("registeredPanelIds")).toStringList();
        m_previousIds = QSet<QString>(previous.cbegin(), previous.cend());
        state->endGroup();
    }

    void WorkspacePanelHistory::observe(const QStringList &ids) {
        const auto previousSize = m_observedIds.size();
        for (const auto &id : ids)
            m_observedIds.insert(id);
        if (m_hasObservedWindow && previousSize == m_observedIds.size())
            return;

        m_hasObservedWindow = true;
        QStringList observed = m_observedIds.values();
        observed.sort();
        auto state = RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.core.workspace"));
        state->setValue(QStringLiteral("hasPreviousSnapshot"), true);
        state->setValue(QStringLiteral("registeredPanelIds"), observed);
        state->endGroup();
    }

    bool WorkspacePanelHistory::isPending(const QString &id) const {
        return m_hasPreviousSnapshot && !m_previousIds.contains(id) && !m_presentedIds.contains(id);
    }

    void WorkspacePanelHistory::markPresented(const QStringList &ids) {
        if (ids.isEmpty())
            return;
        for (const auto &id : ids)
            m_presentedIds.insert(id);
        Q_EMIT presentedChanged();
    }
}

#include "moc_WorkspacePanelHistory.cpp"
