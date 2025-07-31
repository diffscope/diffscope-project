#include "projectwindowworkspacelayout.h"

#include <ranges>
#include <algorithm>
#include <iterator>

namespace Core::Internal {
    ProjectWindowWorkspaceLayout::PanelSpec::operator QVariant() const {
        return QVariantMap{
            {"id",   id  },
            {"dock", dock},
        };
    }
    ProjectWindowWorkspaceLayout::PanelSpec &ProjectWindowWorkspaceLayout::PanelSpec::operator=(const QVariant &variant) {
        auto map = variant.toMap();
        id = map.value("id").toString();
        dock = map.value("dock").toBool();
        return *this;
    }
    ProjectWindowWorkspaceLayout::ViewSpec::operator QVariant() const {
        return QVariantMap{
            {"panels", QVariantList(panels.cbegin(), panels.cend())},
            {"width", width},
            {"height", height},
            {"visibleIndex", visibleIndex},
        };
    }
    ProjectWindowWorkspaceLayout::ViewSpec &ProjectWindowWorkspaceLayout::ViewSpec::operator=(const QVariant &variant) {
        auto map = variant.toMap();
        auto panelsVariantList = map.value("panels").toList();
        panels = QList<PanelSpec>(panelsVariantList.cbegin(), panelsVariantList.cend());
        width = map.value("width").toDouble();
        height = map.value("height").toDouble();
        visibleIndex = map.value("visibleIndex").toInt();
        return *this;
    }
    QString ProjectWindowWorkspaceLayout::name() const {
        return m_name;
    }
    void ProjectWindowWorkspaceLayout::setName(const QString &name) {
        m_name = name;
    }
    ProjectWindowWorkspaceLayout::ViewSpec ProjectWindowWorkspaceLayout::viewSpec(ProjectWindowData::PanelPosition position) const {
        if (position >= m_viewSpecMap.size()) {
            return {};
        }
        return m_viewSpecMap.at(position);
    }
    void ProjectWindowWorkspaceLayout::setViewSpec(ProjectWindowData::PanelPosition position, const ViewSpec &viewSpec) {
        if (position >= m_viewSpecMap.size()) {
            m_viewSpecMap.resize(position + 1);
        }
        m_viewSpecMap[position] = viewSpec;
    }
    QRect ProjectWindowWorkspaceLayout::geometry(const QString &id) const {
        return m_geometryMap.value(id);
    }
    void ProjectWindowWorkspaceLayout::setGeometry(const QString &id, const QRect &geometry) {
        m_geometryMap.insert(id, geometry);
    }
    void ProjectWindowWorkspaceLayout::removeGeometry(const QString &id) {
        m_geometryMap.remove(id);
    }

    QVariant ProjectWindowWorkspaceLayout::toVariant() const {
        return QVariantMap {
            {"name", m_name},
            {"viewSpecMap", QVariantList(m_viewSpecMap.cbegin(), m_viewSpecMap.cend())},
            {"geometry", QVariantHash(m_geometryMap.constKeyValueBegin(), m_geometryMap.constKeyValueEnd())},
        };
    }
    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceLayout::fromVariant(const QVariant &variant) {
        auto map = variant.toMap();
        if (!map.contains("name") || !map.contains("viewSpecMap") || !map.contains("geometry")) {
            return {};
        }
        auto name = map.value("name").toString();
        if (name.isEmpty()) {
            return {};
        }
        ProjectWindowWorkspaceLayout layout;
        layout.m_name = name;
        auto viewSpecVariantList = map.value("viewSpecMap").toList();
        layout.m_viewSpecMap = QList<ViewSpec>(viewSpecVariantList.cbegin(), viewSpecVariantList.cend());
        auto geometryVariantHash = map.value("geometry").toHash();
        auto a = std::ranges::subrange(geometryVariantHash.constKeyValueBegin(), geometryVariantHash.constKeyValueEnd()) | std::views::transform([](const auto &v) {
            return std::make_pair(v.first, v.second.toRect());
        });
        layout.m_geometryMap = QHash<QString, QRect>(a.begin(), a.end());
        return layout;
    }
}