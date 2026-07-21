#include "SourcesPickerModel.h"
#include "SourcesPickerModel_p.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <numeric>
#include <utility>

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

#include <nlohmann/json.hpp>
#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>
#include <opendspxserializer/jsonconverterv1.h>
#include <opendspxserializer/serializer.h>

#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/Sources.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/SingerInfo.h>
#include <coreplugin/SingerRegistry.h>
#include <coreplugin/internal/jsonutils.h>

namespace Core {

    namespace {

        QString workspaceNameFor(const opendspx::Singer &singer) {
            const auto scope = singer.workspace.find("diffscope");
            if (scope == singer.workspace.end() || !scope->second.is_object())
                return {};
            const auto name = scope->second.find("name");
            if (name == scope->second.end() || !name->is_string())
                return {};
            return QString::fromStdString(name->get<std::string>());
        }

        QString joinedWarnings(const QStringList &warnings) {
            return warnings.join(QLatin1Char('\n'));
        }

        bool singerEquals(const opendspx::SingerRef &left, const opendspx::SingerRef &right) {
            if (!left || !right)
                return left == right;
            if (left->type != right->type || left->extra != right->extra || left->workspace != right->workspace)
                return false;
            if (left->type == opendspx::Singer::Type::Single) {
                return static_cast<const opendspx::SingleSinger &>(*left).id
                       == static_cast<const opendspx::SingleSinger &>(*right).id;
            }
            const auto &leftMixed = static_cast<const opendspx::MixedSinger &>(*left);
            const auto &rightMixed = static_cast<const opendspx::MixedSinger &>(*right);
            if (leftMixed.ratio != rightMixed.ratio || leftMixed.singers.size() != rightMixed.singers.size())
                return false;
            for (std::size_t i = 0; i < leftMixed.singers.size(); ++i) {
                if (!singerEquals(leftMixed.singers[i], rightMixed.singers[i]))
                    return false;
            }
            return true;
        }

        bool singerTreeSerializable(const opendspx::SingerRef &singer) {
            if (!singer)
                return false;
            if (singer->type == opendspx::Singer::Type::Single)
                return true;
            if (singer->type != opendspx::Singer::Type::Mixed)
                return false;
            const auto &mixed = static_cast<const opendspx::MixedSinger &>(*singer);
            return std::ranges::all_of(mixed.singers, singerTreeSerializable);
        }

    }

    opendspx::SingerRef SourcesPickerModelPrivate::cloneSinger(const opendspx::SingerRef &source) {
        if (!source)
            return {};
        switch (source->type) {
            case opendspx::Singer::Type::Single: {
                const auto &single = static_cast<const opendspx::SingleSinger &>(*source);
                return std::make_shared<opendspx::SingleSinger>(single.id, single.extra, single.workspace);
            }
            case opendspx::Singer::Type::Mixed: {
                const auto &mixed = static_cast<const opendspx::MixedSinger &>(*source);
                std::vector<opendspx::SingerRef> children;
                children.reserve(mixed.singers.size());
                for (const auto &child : mixed.singers)
                    children.push_back(cloneSinger(child));
                return std::make_shared<opendspx::MixedSinger>(std::move(children), mixed.ratio, mixed.extra, mixed.workspace);
            }
        }
        return {};
    }

    std::unique_ptr<SourcesPickerModelPrivate::Node> SourcesPickerModelPrivate::buildNode(const opendspx::SingerRef &singer, Node *parent) {
        auto node = std::make_unique<Node>();
        node->id = nextNodeId++;
        node->singer = singer;
        node->parent = parent;
        if (singer && singer->type == opendspx::Singer::Type::Mixed) {
            const auto &mixed = static_cast<const opendspx::MixedSinger &>(*singer);
            node->children.reserve(mixed.singers.size());
            for (const auto &child : mixed.singers)
                node->children.push_back(buildNode(child, node.get()));
        }
        return node;
    }

    void SourcesPickerModelPrivate::rebuildNodeLookup() {
        nodesById.clear();
        const auto addNodes = [this](const auto &self, const NodeList &nodes) -> void {
            for (const auto &node : nodes) {
                nodesById.insert(node->id, node.get());
                self(self, node->children);
            }
        };
        addNodes(addNodes, roots);
    }

    QVariant SourcesPickerModelPrivate::singerTree(const Node *node) {
        if (!node || !node->singer)
            return {};
        if (node->singer->type == opendspx::Singer::Type::Single) {
            const auto &single = static_cast<const opendspx::SingleSinger &>(*node->singer);
            return QString::fromStdString(single.id);
        }
        QVariantList result;
        result.reserve(static_cast<qsizetype>(node->children.size()));
        for (const auto &child : node->children)
            result.append(singerTree(child.get()));
        return result;
    }

    bool SourcesPickerModelPrivate::rawRatioValid(const Node *node) {
        const auto *mixed = node && node->singer && node->singer->type == opendspx::Singer::Type::Mixed
                                ? &static_cast<const opendspx::MixedSinger &>(*node->singer)
                                : nullptr;
        if (!mixed)
            return false;
        const auto expected = node->children.empty() ? std::size_t{} : node->children.size() - 1;
        if (mixed->ratio.size() != expected)
            return false;
        double sum = 0.0;
        for (const double value : mixed->ratio) {
            if (!std::isfinite(value) || value < 0.0 || value > 1.0)
                return false;
            sum += value;
        }
        return std::isfinite(sum) && sum <= 1.0 + 1e-12;
    }

    SourcesPickerModelPrivate::Node *SourcesPickerModelPrivate::nodeForIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.model() != q_ptr)
            return nullptr;
        return nodesById.value(static_cast<quint64>(index.internalId()), nullptr);
    }

    SourcesPickerModelPrivate::NodeList *SourcesPickerModelPrivate::siblingsFor(Node *node) {
        return node && node->parent ? &node->parent->children : &roots;
    }

    const SourcesPickerModelPrivate::NodeList *SourcesPickerModelPrivate::siblingsFor(const Node *node) const {
        return node && node->parent ? &node->parent->children : &roots;
    }

    int SourcesPickerModelPrivate::rowOf(const Node *node) const {
        if (!node)
            return -1;
        const auto *siblings = siblingsFor(node);
        const auto it = std::find_if(siblings->begin(), siblings->end(), [node](const auto &candidate) {
            return candidate.get() == node;
        });
        return it == siblings->end() ? -1 : static_cast<int>(std::distance(siblings->begin(), it));
    }

    QModelIndex SourcesPickerModelPrivate::indexForNode(const Node *node) const {
        const int row = rowOf(node);
        return row < 0 ? QModelIndex{} : q_ptr->createIndex(row, 0, quintptr(node->id));
    }

    opendspx::MixedSinger *SourcesPickerModelPrivate::mixedSinger(Node *node) const {
        return node && node->singer && node->singer->type == opendspx::Singer::Type::Mixed
                   ? &static_cast<opendspx::MixedSinger &>(*node->singer)
                   : nullptr;
    }

    const opendspx::MixedSinger *SourcesPickerModelPrivate::mixedSinger(const Node *node) const {
        return node && node->singer && node->singer->type == opendspx::Singer::Type::Mixed
                   ? &static_cast<const opendspx::MixedSinger &>(*node->singer)
                   : nullptr;
    }

    void SourcesPickerModelPrivate::syncMixedChildren(Node *node) {
        auto *mixed = mixedSinger(node);
        if (!mixed)
            return;
        mixed->singers.clear();
        mixed->singers.reserve(node->children.size());
        for (const auto &child : node->children)
            mixed->singers.push_back(child->singer);
    }

    void SourcesPickerModelPrivate::connectRegistry() {
        Q_Q(SourcesPickerModel);
        if (!registry)
            return;
        const auto refreshArchitecture = [this](const QString &id) {
            if (id == architectureId)
                refreshFromRegistry();
        };
        const auto refreshSinger = [this](const QString &architecture, const QString &) {
            if (architecture == architectureId)
                refreshFromRegistry();
        };
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::architectureRegistered, q, refreshArchitecture));
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::architectureUpdated, q, refreshArchitecture));
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::architectureRemoved, q, refreshArchitecture));
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::singerRegistered, q, refreshSinger));
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::singerUpdated, q, refreshSinger));
        registryConnections.push_back(QObject::connect(registry, &SingerRegistry::singerRemoved, q, refreshSinger));
        registryConnections.push_back(QObject::connect(registry, &QObject::destroyed, q, [this, q] {
            registryConnections.clear();
            refreshFromRegistry();
        }));
    }

    void SourcesPickerModelPrivate::emitAllDataChanged(const NodeList &nodes) {
        Q_Q(SourcesPickerModel);
        if (nodes.empty())
            return;
        emit q->dataChanged(indexForNode(nodes.front().get()), indexForNode(nodes.back().get()));
        for (const auto &node : nodes)
            emitAllDataChanged(node->children);
    }

    void SourcesPickerModelPrivate::refreshFromRegistry() {
        Q_Q(SourcesPickerModel);
        rebuildValidation();
        emitAllDataChanged(roots);
        ++revision;
        emit q->validationChanged();
        emit q->revisionChanged(revision);
    }

    void SourcesPickerModelPrivate::addIssue(const QString &code, const QString &path, const QString &message) {
        validationIssues.append(QVariantMap{{QStringLiteral("code"), code},
                                            {QStringLiteral("path"), path},
                                            {QStringLiteral("message"), message}});
    }

    void SourcesPickerModelPrivate::addNodeWarning(Node *node, const QString &code, const QString &path, const QString &message) {
        addIssue(code, path, message);
        validations[node].warnings.append(message);
        validations[node].valid = false;
    }

    SourcesPickerModelPrivate::NodeValidation SourcesPickerModelPrivate::validateNode(Node *node, const QString &path) {
        NodeValidation state;
        state.valid = true;
        validations.insert(node, state);
        if (!node || !node->singer) {
            addNodeWarning(node, QStringLiteral("nullSinger"), path, SourcesPickerModel::tr("The singer entry is empty."));
            return validations.value(node);
        }

        switch (node->singer->type) {
            case opendspx::Singer::Type::Single: {
                const auto &single = static_cast<const opendspx::SingleSinger &>(*node->singer);
                const QString id = QString::fromStdString(single.id);
                if (id.isEmpty()) {
                    addNodeWarning(node, QStringLiteral("emptySingerId"), path, SourcesPickerModel::tr("The singer ID is empty."));
                } else if (!registry || !architectureExists || !registry->containsSinger(architectureId, id)) {
                    addNodeWarning(node, QStringLiteral("missingSinger"), path,
                                   SourcesPickerModel::tr("Singer \"%1\" is not registered in the current architecture.").arg(id));
                } else {
                    validations[node].effectiveMixGroup = registry->singerInfo(architectureId, id).mixGroup();
                }
                break;
            }
            case opendspx::Singer::Type::Mixed: {
                if (node->children.empty())
                    addNodeWarning(node, QStringLiteral("emptyMixedSinger"), path, SourcesPickerModel::tr("The mixed singer has no child singers."));

                QString group;
                bool groupInitialized = false;
                bool containsInvalidChild = false;
                for (std::size_t i = 0; i < node->children.size(); ++i) {
                    auto *child = node->children[i].get();
                    const auto childState = validateNode(child, QStringLiteral("%1.singers[%2]").arg(path).arg(i));
                    if (!childState.valid) {
                        validations[node].valid = false;
                        containsInvalidChild = true;
                    }
                    if (!childState.effectiveMixGroup.isEmpty()) {
                        if (!groupInitialized) {
                            group = childState.effectiveMixGroup;
                            groupInitialized = true;
                        } else if (group != childState.effectiveMixGroup) {
                            addNodeWarning(node, QStringLiteral("mixedGroupMismatch"), path,
                                           SourcesPickerModel::tr("The mixed singer contains incompatible mix groups."));
                            break;
                        }
                    } else if (childState.valid) {
                        addNodeWarning(node, QStringLiteral("nonMixableSinger"), path,
                                       SourcesPickerModel::tr("A singer with an empty mix group cannot participate in a mixed singer."));
                        break;
                    }
                }
                if (containsInvalidChild)
                    validations[node].warnings.append(SourcesPickerModel::tr("The mixed singer contains an invalid child singer."));
                validations[node].effectiveMixGroup = group;
                validations[node].ratioValid = rawRatioValid(node);
                if (!validations[node].ratioValid)
                    addNodeWarning(node, QStringLiteral("invalidRatio"), path, SourcesPickerModel::tr("The mixed singer ratio is invalid."));
                break;
            }
        }
        return validations.value(node);
    }

    void SourcesPickerModelPrivate::rebuildValidation() {
        validations.clear();
        validationIssues.clear();
        rootMixGroup.clear();
        architectureExists = !architectureId.isEmpty() && registry && registry->containsArchitecture(architectureId);

        if (!architectureId.isEmpty() && !architectureExists)
            addIssue(QStringLiteral("missingArchitecture"), QStringLiteral("architectureId"),
                     SourcesPickerModel::tr("Architecture \"%1\" is not registered.").arg(architectureId));

        bool nodesValid = true;
        for (std::size_t i = 0; i < roots.size(); ++i) {
            const auto state = validateNode(roots[i].get(), QStringLiteral("singers[%1]").arg(i));
            nodesValid = nodesValid && state.valid;
        }

        if (roots.size() > 1) {
            bool initialized = false;
            bool compatible = true;
            for (const auto &root : roots) {
                const auto state = validations.value(root.get());
                if (!state.valid || state.effectiveMixGroup.isEmpty()) {
                    compatible = false;
                    break;
                }
                if (!initialized) {
                    rootMixGroup = state.effectiveMixGroup;
                    initialized = true;
                } else if (rootMixGroup != state.effectiveMixGroup) {
                    compatible = false;
                    break;
                }
            }
            if (!compatible) {
                addIssue(QStringLiteral("rootMixGroupMismatch"), QStringLiteral("singers"),
                         SourcesPickerModel::tr("The source singers do not share one non-empty mix group."));
                for (const auto &root : roots) {
                    validations[root.get()].valid = false;
                    validations[root.get()].warnings.append(SourcesPickerModel::tr("The source singer is not compatible with the root mix group."));
                }
                nodesValid = false;
                rootMixGroup.clear();
            }
        } else if (roots.size() == 1) {
            rootMixGroup = validations.value(roots.front().get()).effectiveMixGroup;
        }

        valid = !architectureId.isEmpty() && architectureExists && !roots.empty() && nodesValid && validationIssues.isEmpty();
    }

    QString SourcesPickerModelPrivate::registeredSingerName(const QString &singerId) const {
        if (!registry || !architectureExists || !registry->containsSinger(architectureId, singerId))
            return {};
        return registry->singerInfo(architectureId, singerId).name();
    }

    QString SourcesPickerModelPrivate::displayName(const Node *node) const {
        if (!node || !node->singer)
            return SourcesPickerModel::tr("Invalid singer");
        if (node->singer->type == opendspx::Singer::Type::Single) {
            const auto &single = static_cast<const opendspx::SingleSinger &>(*node->singer);
            const QString id = QString::fromStdString(single.id);
            const QString name = registeredSingerName(id);
            return name.isEmpty() ? (id.isEmpty() ? SourcesPickerModel::tr("Unnamed singer") : id) : name;
        }
        const QString explicitName = workspaceNameFor(*node->singer);
        if (!explicitName.isEmpty())
            return explicitName;
        QStringList names;
        names.reserve(static_cast<qsizetype>(node->children.size()));
        for (const auto &child : node->children)
            names.append(displayName(child.get()));
        return SourcesPickerModel::tr("Mixed singer (%1)").arg(names.join(SourcesPickerModel::tr(", ")));
    }

    QString SourcesPickerModelPrivate::firstLeafSingerId(const Node *node) const {
        if (!node || !node->singer)
            return {};
        if (node->singer->type == opendspx::Singer::Type::Single)
            return QString::fromStdString(static_cast<const opendspx::SingleSinger &>(*node->singer).id);
        for (const auto &child : node->children) {
            const QString id = firstLeafSingerId(child.get());
            if (!id.isEmpty())
                return id;
        }
        return {};
    }

    QString SourcesPickerModelPrivate::firstLeafSingerId(const NodeList &nodes) const {
        for (const auto &node : nodes) {
            const QString id = firstLeafSingerId(node.get());
            if (!id.isEmpty())
                return id;
        }
        return {};
    }

    QList<double> SourcesPickerModelPrivate::displayRatios(const Node *mixedNode) const {
        QList<double> result;
        const auto count = static_cast<qsizetype>(mixedNode ? mixedNode->children.size() : 0);
        if (count <= 0)
            return result;
        if (!rawRatioValid(mixedNode)) {
            result.fill(1.0 / static_cast<double>(count), count);
            return result;
        }
        const auto *mixed = mixedSinger(mixedNode);
        result.reserve(count);
        double sum = 0.0;
        for (const double value : mixed->ratio) {
            result.append(value);
            sum += value;
        }
        result.append(std::max(0.0, 1.0 - sum));
        return result;
    }

    void SourcesPickerModelPrivate::writeRatios(Node *mixedNode, QList<double> ratios) {
        auto *mixed = mixedSinger(mixedNode);
        if (!mixed || ratios.isEmpty())
            return;
        double sum = 0.0;
        for (double &value : ratios) {
            value = std::isfinite(value) ? std::max(0.0, value) : 0.0;
            sum += value;
        }
        if (sum <= 1e-12) {
            ratios.fill(1.0 / static_cast<double>(ratios.size()));
        } else {
            for (double &value : ratios)
                value /= sum;
        }
        mixed->ratio.clear();
        double storedSum = 0.0;
        for (qsizetype i = 0; i + 1 < ratios.size(); ++i) {
            const double value = std::clamp(ratios[i], 0.0, std::max(0.0, 1.0 - storedSum));
            mixed->ratio.push_back(value);
            storedSum += value;
        }
    }

    bool SourcesPickerModelPrivate::reject(const QString &message) {
        Q_Q(SourcesPickerModel);
        emit q->operationRejected(message);
        return false;
    }

    bool SourcesPickerModelPrivate::validateSelection(const QString &architecture, const QString &singerId,
                                                       QString *mixGroup, QJsonValue *defaultExtra) const {
        if (!registry || architecture.isEmpty() || !registry->containsArchitecture(architecture) || singerId.isEmpty()
            || !registry->containsSinger(architecture, singerId))
            return false;
        const SingerInfo info = registry->singerInfo(architecture, singerId);
        if (mixGroup)
            *mixGroup = info.mixGroup();
        if (defaultExtra)
            *defaultExtra = info.defaultExtra();
        return true;
    }

    void SourcesPickerModelPrivate::changed(bool rootCountChanged) {
        Q_Q(SourcesPickerModel);
        rebuildValidation();
        emitAllDataChanged(roots);
        ++revision;
        emit q->singersChanged();
        if (rootCountChanged) {
            emit q->countChanged(static_cast<int>(roots.size()));
            emit q->emptyChanged(roots.empty());
        }
        emit q->validationChanged();
        emit q->revisionChanged(revision);
    }

    SourcesPickerModel::SourcesPickerModel(QObject *parent)
        : QAbstractItemModel(parent), d_ptr(new SourcesPickerModelPrivate) {
        Q_D(SourcesPickerModel);
        d->q_ptr = this;
        d->registry = CoreInterface::singerRegistry();
        d->connectRegistry();
        d->rebuildValidation();
    }

    SourcesPickerModel::~SourcesPickerModel() = default;

    QString SourcesPickerModel::architectureId() const {
        Q_D(const SourcesPickerModel);
        return d->architectureId;
    }

    void SourcesPickerModel::setArchitectureId(const QString &architectureId) {
        Q_D(SourcesPickerModel);
        if (d->architectureId == architectureId)
            return;
        d->architectureId = architectureId;
        emit architectureIdChanged(architectureId);
        d->refreshFromRegistry();
    }

    QList<opendspx::SingerRef> SourcesPickerModel::singers() const {
        Q_D(const SourcesPickerModel);
        QList<opendspx::SingerRef> result;
        result.reserve(static_cast<qsizetype>(d->roots.size()));
        for (const auto &root : d->roots)
            result.append(root->singer);
        return result;
    }

    void SourcesPickerModel::setSingers(const QList<opendspx::SingerRef> &singers) {
        Q_D(SourcesPickerModel);
        if (singers.size() == static_cast<qsizetype>(d->roots.size())) {
            bool equal = true;
            for (qsizetype i = 0; i < singers.size(); ++i) {
                if (!singerEquals(singers[i], d->roots[static_cast<std::size_t>(i)]->singer)) {
                    equal = false;
                    break;
                }
            }
            if (equal)
                return;
        }
        const int oldCount = static_cast<int>(d->roots.size());
        beginResetModel();
        d->roots.clear();
        d->roots.reserve(static_cast<std::size_t>(singers.size()));
        for (const auto &singer : singers)
            d->roots.push_back(d->buildNode(SourcesPickerModelPrivate::cloneSinger(singer), nullptr));
        d->rebuildNodeLookup();
        endResetModel();
        d->changed(oldCount != static_cast<int>(d->roots.size()));
    }

    void SourcesPickerModel::fromSources(dspx::Sources *sources) {
        if (!sources) {
            setArchitectureId({});
            setSingers({});
            return;
        }

        QList<opendspx::SingerRef> singers;
        const auto sourceSingers = sources->singers()->toOpenDSPX();
        singers.reserve(static_cast<qsizetype>(sourceSingers.size()));
        for (const auto &singer : sourceSingers)
            singers.append(singer);
        setArchitectureId(sources->category());
        setSingers(singers);
    }

    void SourcesPickerModel::fromDefaultSinger(const QString &singerId) {
        Q_D(SourcesPickerModel);
        QList<opendspx::SingerRef> defaultSingers;
        if (d->registry && d->registry->containsSinger(d->architectureId, singerId)) {
            const auto info = d->registry->singerInfo(d->architectureId, singerId);
            defaultSingers.append(std::make_shared<opendspx::SingleSinger>(
                singerId.toStdString(), Internal::JsonUtils::fromQJsonValue(info.defaultExtra())));
        }
        setSingers(defaultSingers);
    }

    QByteArray SourcesPickerModel::serialize() const {
        std::vector<opendspx::SingerRef> singerVector;
        const auto currentSingers = singers();
        singerVector.reserve(static_cast<std::size_t>(currentSingers.size()));
        for (const auto &singer : currentSingers) {
            if (!singerTreeSerializable(singer))
                return {};
            singerVector.push_back(singer);
        }

        try {
            opendspx::SerializationErrorList errors;
            const auto singersJson = opendspx::JsonConverterV1::toJson(singerVector, errors, {});
            if (errors.containsFatal() || errors.containsError())
                return {};

            QJsonObject object;
            object.insert(QStringLiteral("architectureId"), architectureId());
            object.insert(QStringLiteral("singers"), Internal::JsonUtils::toQJsonValue(singersJson));
            return QJsonDocument(object).toJson(QJsonDocument::Compact);
        } catch (const std::exception &) {
            return {};
        }
    }

    bool SourcesPickerModel::deserialize(const QByteArray &data) {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            return false;

        const auto object = document.object();
        const auto architectureIdValue = object.value(QStringLiteral("architectureId"));
        const auto singersValue = object.value(QStringLiteral("singers"));
        if (!architectureIdValue.isString() || !singersValue.isArray())
            return false;

        try {
            opendspx::SerializationErrorList errors;
            const auto singerVector = opendspx::JsonConverterV1::fromJson<std::vector<opendspx::SingerRef>>(
                Internal::JsonUtils::fromQJsonValue(singersValue), errors, {});
            if (errors.containsFatal() || errors.containsError()
                || !std::ranges::all_of(singerVector, singerTreeSerializable)) {
                return false;
            }

            QList<opendspx::SingerRef> deserializedSingers;
            deserializedSingers.reserve(static_cast<qsizetype>(singerVector.size()));
            for (const auto &singer : singerVector)
                deserializedSingers.append(singer);

            setArchitectureId(architectureIdValue.toString());
            setSingers(deserializedSingers);
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }

    int SourcesPickerModel::count() const {
        Q_D(const SourcesPickerModel);
        return static_cast<int>(d->roots.size());
    }

    bool SourcesPickerModel::empty() const {
        return count() == 0;
    }

    bool SourcesPickerModel::valid() const {
        Q_D(const SourcesPickerModel);
        return d->valid;
    }

    bool SourcesPickerModel::architectureExists() const {
        Q_D(const SourcesPickerModel);
        return d->architectureExists;
    }

    bool SourcesPickerModel::canAppend() const {
        Q_D(const SourcesPickerModel);
        return d->valid && !d->roots.empty() && !d->rootMixGroup.isEmpty();
    }

    QString SourcesPickerModel::mixGroup() const {
        Q_D(const SourcesPickerModel);
        return d->rootMixGroup;
    }

    QVariantList SourcesPickerModel::validationIssues() const {
        Q_D(const SourcesPickerModel);
        return d->validationIssues;
    }

    qulonglong SourcesPickerModel::revision() const {
        Q_D(const SourcesPickerModel);
        return d->revision;
    }

    QModelIndex SourcesPickerModel::index(int row, int column, const QModelIndex &parent) const {
        Q_D(const SourcesPickerModel);
        if (column != 0 || row < 0)
            return {};
        const auto *parentNode = d->nodeForIndex(parent);
        if (parent.isValid() && !parentNode)
            return {};
        const auto &nodes = parentNode ? parentNode->children : d->roots;
        if (row >= static_cast<int>(nodes.size()))
            return {};
        return createIndex(row, column, quintptr(nodes[static_cast<std::size_t>(row)]->id));
    }

    QModelIndex SourcesPickerModel::parent(const QModelIndex &child) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(child);
        return node && node->parent ? d->indexForNode(node->parent) : QModelIndex{};
    }

    int SourcesPickerModel::rowCount(const QModelIndex &parent) const {
        Q_D(const SourcesPickerModel);
        if (parent.column() > 0)
            return 0;
        const auto *parentNode = d->nodeForIndex(parent);
        if (parent.isValid() && !parentNode)
            return 0;
        return static_cast<int>(parentNode ? parentNode->children.size() : d->roots.size());
    }

    int SourcesPickerModel::columnCount(const QModelIndex &) const {
        return 1;
    }

    QVariant SourcesPickerModel::data(const QModelIndex &index, int role) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        if (!node)
            return {};
        const auto state = d->validations.value(node);
        switch (role) {
            case Qt::DisplayRole:
            case DisplayNameRole:
                return d->displayName(node);
            case SingerTypeRole:
                return singerType(index);
            case SingerIdRole:
                return singerId(index);
            case ExtraRole:
                return node->singer ? Internal::JsonUtils::toQJsonValue(node->singer->extra) : QJsonValue{};
            case SingerTreeRole: {
                const QVariant tree = SourcesPickerModelPrivate::singerTree(node);
                return node->singer && node->singer->type == opendspx::Singer::Type::Single
                           ? QVariant(QVariantList{tree})
                           : tree;
            }
            case WorkspaceNameRole:
                return node->singer ? workspaceNameFor(*node->singer) : QString{};
            case RatioRole: {
                if (!node->parent)
                    return 1.0;
                const auto values = d->displayRatios(node->parent);
                const int row = d->rowOf(node);
                return row >= 0 && row < values.size() ? values[row] : 0.0;
            }
            case SingerValidRole:
                return state.valid;
            case WarningTextRole:
                return joinedWarnings(state.warnings);
            case EffectiveMixGroupRole:
                return state.effectiveMixGroup;
            case ModelIndexRole:
                return index;
            default:
                return {};
        }
    }

    QHash<int, QByteArray> SourcesPickerModel::roleNames() const {
        return {{SingerTypeRole, "singerType"},
                {SingerIdRole, "singerId"},
                {ExtraRole, "extra"},
                {SingerTreeRole, "singerTree"},
                {WorkspaceNameRole, "workspaceName"},
                {DisplayNameRole, "displayName"},
                {RatioRole, "ratio"},
                {SingerValidRole, "singerValid"},
                {WarningTextRole, "warningText"},
                {EffectiveMixGroupRole, "effectiveMixGroup"},
                {ModelIndexRole, "modelIndex"}};
    }

    QModelIndex SourcesPickerModel::modelIndex(int row, const QModelIndex &parent) const {
        return index(row, 0, parent);
    }

    bool SourcesPickerModel::indexAlive(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        return d->nodeForIndex(index) != nullptr;
    }

    int SourcesPickerModel::childCount(const QModelIndex &parent) const {
        return rowCount(parent);
    }

    SourcesPickerModel::SingerType SourcesPickerModel::singerType(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        if (!node || !node->singer)
            return InvalidSinger;
        return node->singer->type == opendspx::Singer::Type::Single ? SingleSinger : MixedSinger;
    }

    QString SourcesPickerModel::singerId(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        return node && node->singer && node->singer->type == opendspx::Singer::Type::Single
                   ? QString::fromStdString(static_cast<const opendspx::SingleSinger &>(*node->singer).id)
                   : QString{};
    }

    QJsonValue SourcesPickerModel::singerExtra(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        return node && node->singer ? Internal::JsonUtils::toQJsonValue(node->singer->extra) : QJsonValue{};
    }

    QString SourcesPickerModel::workspaceName(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        return node && node->singer ? workspaceNameFor(*node->singer) : QString{};
    }

    QString SourcesPickerModel::displayName(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        return d->displayName(d->nodeForIndex(index));
    }

    bool SourcesPickerModel::singerValid(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        const auto *node = d->nodeForIndex(index);
        return node && d->validations.value(node).valid;
    }

    QString SourcesPickerModel::warningText(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        return joinedWarnings(d->validations.value(d->nodeForIndex(index)).warnings);
    }

    QString SourcesPickerModel::effectiveMixGroup(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        return d->validations.value(d->nodeForIndex(index)).effectiveMixGroup;
    }

    QString SourcesPickerModel::firstLeafSingerId(const QModelIndex &index) const {
        Q_D(const SourcesPickerModel);
        return index.isValid() ? d->firstLeafSingerId(d->nodeForIndex(index)) : d->firstLeafSingerId(d->roots);
    }

    QVariantList SourcesPickerModel::ratios(const QModelIndex &mixedSingerIndex) const {
        Q_D(const SourcesPickerModel);
        QVariantList result;
        const auto values = d->displayRatios(d->nodeForIndex(mixedSingerIndex));
        result.reserve(values.size());
        for (const double value : values)
            result.append(value);
        return result;
    }

    bool SourcesPickerModel::trySelectInitialSinger(const QString &architecture, const QString &singerId) {
        Q_D(SourcesPickerModel);
        QString mixGroup;
        QJsonValue defaultExtra;
        if (!d->validateSelection(architecture, singerId, &mixGroup, &defaultExtra))
            return d->reject(tr("The selected singer is no longer available."));
        if (!d->architectureId.isEmpty() && !d->roots.empty())
            return d->reject(tr("An initial singer can only be selected for an empty source selection."));

        const bool architectureChanged = d->architectureId != architecture;
        const int oldCount = static_cast<int>(d->roots.size());
        beginResetModel();
        d->architectureId = architecture;
        d->roots.clear();
        auto singer = std::make_shared<opendspx::SingleSinger>(
            singerId.toStdString(), Internal::JsonUtils::fromQJsonValue(defaultExtra));
        d->roots.push_back(d->buildNode(singer, nullptr));
        d->rebuildNodeLookup();
        endResetModel();
        if (architectureChanged)
            emit architectureIdChanged(architecture);
        d->changed(oldCount != 1);
        return true;
    }

    bool SourcesPickerModel::tryAppendSinger(const QString &architecture, const QString &singerId) {
        Q_D(SourcesPickerModel);
        if (!d->valid || d->roots.empty() || d->rootMixGroup.isEmpty())
            return d->reject(tr("The current source list cannot accept another singer."));
        if (architecture != d->architectureId)
            return d->reject(tr("The selected singer belongs to a different architecture."));
        QString mixGroup;
        QJsonValue defaultExtra;
        if (!d->validateSelection(architecture, singerId, &mixGroup, &defaultExtra))
            return d->reject(tr("The selected singer is no longer available."));
        if (mixGroup.isEmpty() || mixGroup != d->rootMixGroup)
            return d->reject(tr("The selected singer is not compatible with the current mix group."));

        const int row = static_cast<int>(d->roots.size());
        beginInsertRows({}, row, row);
        auto singer = std::make_shared<opendspx::SingleSinger>(
            singerId.toStdString(), Internal::JsonUtils::fromQJsonValue(defaultExtra));
        d->roots.push_back(d->buildNode(singer, nullptr));
        d->rebuildNodeLookup();
        endInsertRows();
        d->changed(true);
        return true;
    }

    bool SourcesPickerModel::tryAppendSingerToMixed(const QModelIndex &mixedSingerIndex, const QString &architecture,
                                                     const QString &singerId) {
        Q_D(SourcesPickerModel);
        auto *mixedNode = d->nodeForIndex(mixedSingerIndex);
        if (!d->mixedSinger(mixedNode) || !d->validations.value(mixedNode).valid)
            return d->reject(tr("The current mixed singer cannot accept entries while it is invalid."));
        if (architecture != d->architectureId)
            return d->reject(tr("The selected singer belongs to a different architecture."));

        QString mixGroup;
        QJsonValue defaultExtra;
        if (!d->validateSelection(architecture, singerId, &mixGroup, &defaultExtra))
            return d->reject(tr("The selected singer is no longer available."));
        const QString requiredGroup = d->validations.value(mixedNode).effectiveMixGroup;
        if (requiredGroup.isEmpty() || mixGroup != requiredGroup)
            return d->reject(tr("The selected singer is not compatible with the current mix group."));

        auto shares = d->displayRatios(mixedNode);
        const int oldCount = static_cast<int>(mixedNode->children.size());
        const double retainedShare = static_cast<double>(oldCount) / static_cast<double>(oldCount + 1);
        for (double &share : shares)
            share *= retainedShare;
        shares.append(1.0 / static_cast<double>(oldCount + 1));

        beginInsertRows(mixedSingerIndex, oldCount, oldCount);
        auto singer = std::make_shared<opendspx::SingleSinger>(
            singerId.toStdString(), Internal::JsonUtils::fromQJsonValue(defaultExtra));
        mixedNode->children.push_back(d->buildNode(singer, mixedNode));
        d->syncMixedChildren(mixedNode);
        d->writeRatios(mixedNode, shares);
        d->rebuildNodeLookup();
        endInsertRows();
        d->changed();
        return true;
    }

    bool SourcesPickerModel::tryReplaceSinger(const QModelIndex &index, const QString &architecture, const QString &singerId) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        auto *parentNode = node ? node->parent : nullptr;
        if (!node || !parentNode || !d->mixedSinger(parentNode) || !d->validations.value(parentNode).valid)
            return d->reject(tr("The current mixed singer cannot replace entries while it is invalid."));
        if (architecture != d->architectureId)
            return d->reject(tr("The selected singer belongs to a different architecture."));
        QString mixGroup;
        QJsonValue defaultExtra;
        if (!d->validateSelection(architecture, singerId, &mixGroup, &defaultExtra))
            return d->reject(tr("The selected singer is no longer available."));
        const QString requiredGroup = d->validations.value(node).effectiveMixGroup;
        if (requiredGroup.isEmpty() || mixGroup != requiredGroup)
            return d->reject(tr("The selected singer is not compatible with the current mix group."));

        auto singer = std::make_shared<opendspx::SingleSinger>(
            singerId.toStdString(), Internal::JsonUtils::fromQJsonValue(defaultExtra));
        if (singerEquals(node->singer, singer))
            return true;
        const int childCount = static_cast<int>(node->children.size());
        if (childCount > 0)
            beginRemoveRows(d->indexForNode(node), 0, childCount - 1);
        node->singer = std::move(singer);
        node->children.clear();
        d->syncMixedChildren(parentNode);
        d->rebuildNodeLookup();
        if (childCount > 0)
            endRemoveRows();
        d->changed();
        return true;
    }

    bool SourcesPickerModel::removeSinger(const QModelIndex &index) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        if (!node)
            return false;
        auto *siblings = d->siblingsFor(node);
        if (node->parent && siblings->size() <= 1)
            return d->reject(tr("A mixed singer must retain at least one child singer."));
        const int row = d->rowOf(node);
        auto *parentNode = node->parent;
        QList<double> shares;
        if (parentNode)
            shares = d->displayRatios(parentNode);
        const QModelIndex parentIndex = parentNode ? d->indexForNode(parentNode) : QModelIndex{};
        beginRemoveRows(parentIndex, row, row);
        siblings->erase(siblings->begin() + row);
        if (parentNode) {
            shares.removeAt(row);
            d->syncMixedChildren(parentNode);
            d->writeRatios(parentNode, shares);
        }
        d->rebuildNodeLookup();
        endRemoveRows();
        d->changed(!parentNode);
        return true;
    }

    bool SourcesPickerModel::moveSinger(const QModelIndex &index, int destinationRow) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        if (!node)
            return false;
        auto *siblings = d->siblingsFor(node);
        const int sourceRow = d->rowOf(node);
        destinationRow = std::clamp(destinationRow, 0, static_cast<int>(siblings->size()) - 1);
        if (sourceRow == destinationRow)
            return true;
        auto *parentNode = node->parent;
        QList<double> shares;
        if (parentNode)
            shares = d->displayRatios(parentNode);
        const QModelIndex parentIndex = parentNode ? d->indexForNode(parentNode) : QModelIndex{};
        if (!beginMoveRows(parentIndex, sourceRow, sourceRow, parentIndex,
                           destinationRow > sourceRow ? destinationRow + 1 : destinationRow))
            return false;
        auto moving = std::move((*siblings)[static_cast<std::size_t>(sourceRow)]);
        siblings->erase(siblings->begin() + sourceRow);
        siblings->insert(siblings->begin() + destinationRow, std::move(moving));
        if (parentNode) {
            shares.move(sourceRow, destinationRow);
            d->syncMixedChildren(parentNode);
            d->writeRatios(parentNode, shares);
        }
        endMoveRows();
        d->changed();
        return true;
    }

    bool SourcesPickerModel::wrapSingerInMixed(const QModelIndex &index) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        if (!node || !d->validations.value(node).valid || d->validations.value(node).effectiveMixGroup.isEmpty())
            return d->reject(tr("This singer cannot participate in a mixed singer."));
        const auto originalSinger = node->singer;
        auto mixed = std::make_shared<opendspx::MixedSinger>(std::vector<opendspx::SingerRef>{originalSinger});
        auto *parentNode = node->parent;
        auto child = std::make_unique<SourcesPickerModelPrivate::Node>();
        child->id = d->nextNodeId++;
        child->singer = originalSinger;
        child->parent = node;
        const bool wrappingLeaf = node->children.empty();
        const QModelIndex nodeIndex = d->indexForNode(node);
        if (wrappingLeaf) {
            beginInsertRows(nodeIndex, 0, 0);
            node->singer = std::move(mixed);
            node->children.push_back(std::move(child));
            if (parentNode)
                d->syncMixedChildren(parentNode);
            d->rebuildNodeLookup();
            endInsertRows();
        } else {
            const int oldChildCount = static_cast<int>(node->children.size());
            beginRemoveRows(nodeIndex, 0, oldChildCount - 1);
            child->children = std::move(node->children);
            for (const auto &grandchild : child->children)
                grandchild->parent = child.get();
            d->rebuildNodeLookup();
            endRemoveRows();

            node->singer = std::move(mixed);
            beginInsertRows(nodeIndex, 0, 0);
            node->children.push_back(std::move(child));
            if (parentNode)
                d->syncMixedChildren(parentNode);
            d->rebuildNodeLookup();
            endInsertRows();
        }
        d->changed();
        return true;
    }

    bool SourcesPickerModel::setSingerExtra(const QModelIndex &index, const QJsonValue &extra) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        if (!node || !node->singer)
            return false;
        const auto converted = Internal::JsonUtils::fromQJsonValue(extra);
        if (node->singer->extra == converted)
            return true;
        node->singer->extra = converted;
        emit dataChanged(index, index, {ExtraRole});
        d->changed();
        return true;
    }

    bool SourcesPickerModel::setMixedName(const QModelIndex &index, const QString &name) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(index);
        auto *mixed = d->mixedSinger(node);
        if (!mixed)
            return false;
        if (workspaceNameFor(*mixed) == name)
            return true;
        auto &scope = mixed->workspace["diffscope"];
        if (!scope.is_object())
            scope = nlohmann::json::object();
        scope["name"] = name.toStdString();
        emit dataChanged(index, index, {WorkspaceNameRole, DisplayNameRole});
        d->changed();
        return true;
    }

    bool SourcesPickerModel::setSingerRatio(const QModelIndex &mixedSingerIndex, int row, double ratio) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(mixedSingerIndex);
        const int childCount = node ? static_cast<int>(node->children.size()) : 0;
        if (!d->mixedSinger(node) || childCount < 2 || row < 0 || row >= childCount || !std::isfinite(ratio))
            return false;

        if (row + 1 < childCount)
            return setAdjacentRatios(mixedSingerIndex, row, ratio);

        const auto shares = d->displayRatios(node);
        const double pairSum = shares[row - 1] + shares[row];
        return setAdjacentRatios(mixedSingerIndex, row - 1, pairSum - ratio);
    }

    bool SourcesPickerModel::setAdjacentRatios(const QModelIndex &mixedSingerIndex, int leftRow, double leftRatio) {
        Q_D(SourcesPickerModel);
        auto *node = d->nodeForIndex(mixedSingerIndex);
        if (!d->mixedSinger(node) || leftRow < 0 || leftRow + 1 >= static_cast<int>(node->children.size()))
            return false;
        auto shares = d->displayRatios(node);
        const double pairSum = shares[leftRow] + shares[leftRow + 1];
        shares[leftRow] = std::clamp(leftRatio, 0.0, pairSum);
        shares[leftRow + 1] = pairSum - shares[leftRow];
        const auto oldRatio = d->mixedSinger(node)->ratio;
        d->writeRatios(node, shares);
        if (oldRatio == d->mixedSinger(node)->ratio)
            return true;
        emit dataChanged(index(0, 0, mixedSingerIndex), index(rowCount(mixedSingerIndex) - 1, 0, mixedSingerIndex), {RatioRole});
        d->changed();
        return true;
    }

}

#include "moc_SourcesPickerModel.cpp"
