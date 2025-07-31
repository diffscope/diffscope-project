#ifndef DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACELAYOUT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACELAYOUT_H

#include <coreplugin/internal/projectwindowdata.h>

#include <utility>

namespace Core::Internal {

    class ProjectWindowWorkspaceLayout {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
    public:

        struct PanelSpec {
            inline PanelSpec() : dock(false) {
            }
            inline PanelSpec(const QVariant &other) { *this = other; }
            inline PanelSpec(QString id, bool dock) : id(id), dock(dock) {
            }
            QString id;
            bool dock;
            operator QVariant() const;
            PanelSpec &operator=(const QVariant &variant);
        };

        struct ViewSpec {
            inline ViewSpec() : width(0), height(0), visibleIndex(0) {
            }
            inline ViewSpec(const QVariant &other) { *this = other; }
            inline ViewSpec(QList<PanelSpec> panels, double width, double height, int visibleIndex) : panels(std::move(panels)), width(width), height(height), visibleIndex(visibleIndex) {
            }
            QList<PanelSpec> panels;
            double width;
            double height;
            int visibleIndex;
            operator QVariant() const;
            ViewSpec &operator=(const QVariant &variant);
        };

        QString name() const;
        void setName(const QString &name);

        inline bool isValid() const {
            return !m_name.isEmpty();
        }

        bool operator==(const ProjectWindowWorkspaceLayout &o) const {
            return m_name == o.m_name;
        }

        Q_INVOKABLE ViewSpec viewSpec(ProjectWindowData::PanelPosition position) const;
        Q_INVOKABLE void setViewSpec(ProjectWindowData::PanelPosition position, const ViewSpec &viewSpec);

        Q_INVOKABLE QRect geometry(const QString &id) const;
        Q_INVOKABLE void setGeometry(const QString &id, const QRect &geometry);
        Q_INVOKABLE void removeGeometry(const QString &id);

        QVariant toVariant() const;
        static ProjectWindowWorkspaceLayout fromVariant(const QVariant &variant);

    private:
        QString m_name;
        QList<ViewSpec> m_viewSpecMap;
        QHash<QString, QRect> m_geometryMap;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTWINDOWWORKSPACELAYOUT_H
