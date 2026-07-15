#ifndef DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_P_H
#define DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_P_H

#include <coreplugin/SourcesPickerModel.h>

#include <memory>
#include <numeric>
#include <vector>

#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QStringList>

#include <opendspx/mixedsinger.h>

namespace Core {

    class SourcesPickerModelPrivate {
        Q_DECLARE_PUBLIC(SourcesPickerModel)

    public:
        struct Node {
            quint64 id{};
            opendspx::SingerRef singer;
            Node *parent{};
            std::vector<std::unique_ptr<Node>> children;
        };

        struct NodeValidation {
            bool valid{};
            bool ratioValid{true};
            QString effectiveMixGroup;
            QStringList warnings;
        };

        using NodeList = std::vector<std::unique_ptr<Node>>;

        SourcesPickerModel *q_ptr{};
        QString architectureId;
        QPointer<SingerRegistry> registry;
        NodeList roots;
        quint64 nextNodeId{1};
        QHash<quint64, Node *> nodesById;
        QHash<const Node *, NodeValidation> validations;
        QVariantList validationIssues;
        bool valid{};
        bool architectureExists{};
        QString rootMixGroup;
        qulonglong revision{};
        std::vector<QMetaObject::Connection> registryConnections;

        static opendspx::SingerRef cloneSinger(const opendspx::SingerRef &source);
        std::unique_ptr<Node> buildNode(const opendspx::SingerRef &singer, Node *parent);
        static QVariant singerTree(const Node *node);
        static bool rawRatioValid(const Node *node);

        void rebuildNodeLookup();
        Node *nodeForIndex(const QModelIndex &index) const;
        NodeList *siblingsFor(Node *node);
        const NodeList *siblingsFor(const Node *node) const;
        int rowOf(const Node *node) const;
        QModelIndex indexForNode(const Node *node) const;
        opendspx::MixedSinger *mixedSinger(Node *node) const;
        const opendspx::MixedSinger *mixedSinger(const Node *node) const;
        void syncMixedChildren(Node *node);

        void disconnectRegistry();
        void connectRegistry();
        void refreshFromRegistry();
        void emitAllDataChanged(const NodeList &nodes);
        void rebuildValidation();
        NodeValidation validateNode(Node *node, const QString &path);
        void addIssue(const QString &code, const QString &path, const QString &message);
        void addNodeWarning(Node *node, const QString &code, const QString &path, const QString &message);

        QString registeredSingerName(const QString &singerId) const;
        QString displayName(const Node *node) const;
        QString firstLeafSingerId(const Node *node) const;
        QString firstLeafSingerId(const NodeList &nodes) const;
        QList<double> displayRatios(const Node *mixedNode) const;
        void writeRatios(Node *mixedNode, QList<double> ratios);

        bool reject(const QString &message);
        bool validateSelection(const QString &architectureId, const QString &singerId, QString *mixGroup, QJsonValue *defaultExtra) const;
        void changed(bool countChanged = false);
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SOURCESPICKERMODEL_P_H
