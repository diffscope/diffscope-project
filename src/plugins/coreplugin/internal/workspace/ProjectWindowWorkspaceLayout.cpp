// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectWindowWorkspaceLayout.h"

#include <functional>

#include <QSet>

#include <SVSCraftQuick/DockingLayout.h>

namespace Core::Internal {
    QString ProjectWindowWorkspaceLayout::name() const {
        return m_name;
    }

    void ProjectWindowWorkspaceLayout::setName(const QString &name) {
        m_name = name;
    }

    QVariantMap ProjectWindowWorkspaceLayout::state() const {
        return m_state;
    }

    void ProjectWindowWorkspaceLayout::setState(const QVariantMap &state) {
        m_state = state;
    }

    bool ProjectWindowWorkspaceLayout::isValid() const {
        if (m_name.isEmpty() || !m_state.contains("mainRoot")
            || !m_state.value("floatingHosts").canConvert<QVariantList>()
            || !m_state.value("panelStates").canConvert<QVariantMap>()
            || m_state.value("floatingHosts").toList().size() > 128)
            return false;

        QSet<QString> panels, shown, hosts;
        hosts.insert("main");
        std::function<bool(const SVS::DockingNodeState &)> unique = [&](const auto &node) {
            for (const auto &side : node.panelIds)
                for (const auto &id : side) {
                    if (panels.contains(id))
                        return false;
                    panels.insert(id);
                }
            for (const auto &id : node.selected)
                if (!id.isEmpty())
                    shown.insert(id);
            for (const auto &child : node.children)
                if (!unique(child))
                    return false;
            return true;
        };
        auto check = [&](const QVariant &root) {
            if (!root.canConvert<QVariantMap>())
                return false;
            auto state = SVS::DockingLayoutState::fromVariant(root.toMap());
            return SVS::DockingLayout::validateState(state).accepted() && (!state.hasRoot || unique(state.root));
        };
        if (!check(m_state.value("mainRoot")))
            return false;
        for (const auto &value : m_state.value("floatingHosts").toList()) {
            auto host = value.toMap();
            auto id = host.value("hostId").toString();
            auto rect = host.value("normalGeometry").toRect();
            if (id.isEmpty() || hosts.contains(id) || host.value("root").toMap().isEmpty() || rect.width() <= 0 || rect.height() <= 0 || !check(host.value("root")))
                return false;
            hosts.insert(id);
        }
        auto active = m_state.value("activePanelId").toString();
        return active.isEmpty() || shown.contains(active);
    }

    QVariant ProjectWindowWorkspaceLayout::toVariant() const {
        auto v = m_state;
        v.insert("name", m_name);
        return v;
    }

    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceLayout::fromVariant(const QVariant &variant) {
        ProjectWindowWorkspaceLayout layout;
        auto map = variant.toMap();
        layout.m_name = map.take("name").toString();
        layout.m_state = map;
        return layout.isValid() ? layout : ProjectWindowWorkspaceLayout{};
    }
}
