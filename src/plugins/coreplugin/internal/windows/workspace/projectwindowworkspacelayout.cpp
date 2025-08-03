#include "projectwindowworkspacelayout.h"

#include <ranges>
#include <algorithm>
#include <iterator>

namespace Core::Internal {
    ProjectWindowWorkspaceLayout::PanelSpec::operator QVariant() const {
        return QVariantMap{
            {"id",   id  },
            {"dock", dock},
            {"opened", opened},
            {"geometry", geometry},
        };
    }
    ProjectWindowWorkspaceLayout::PanelSpec &ProjectWindowWorkspaceLayout::PanelSpec::operator=(const QVariant &variant) {
        auto map = variant.toMap();
        id = map.value("id").toString();
        dock = map.value("dock").toBool();
        opened = map.value("opened").toBool();
        geometry = map.value("geometry").toRect();
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
    void ProjectWindowWorkspaceLayout::setViewSpecFromJavaScript(const QJSValue &viewSpecJSArray) {
        m_viewSpecMap.clear();
        for (int i = ProjectWindowData::LeftTop; i <= ProjectWindowData::BottomRight; i++) {
            auto viewSpec = viewSpecJSArray.property(i).toVariant(QJSValue::ConvertJSObjects);
            setViewSpec(static_cast<ProjectWindowData::PanelPosition>(i), viewSpec);
        }
    }

    QVariant ProjectWindowWorkspaceLayout::toVariant() const {
        return QVariantMap {
            {"name", m_name},
            {"viewSpecMap", QVariantList(m_viewSpecMap.cbegin(), m_viewSpecMap.cend())}
        };
    }
    ProjectWindowWorkspaceLayout ProjectWindowWorkspaceLayout::fromVariant(const QVariant &variant) {
        auto map = variant.toMap();
        if (!map.contains("name") || !map.contains("viewSpecMap")) {
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
        return layout;
    }
}