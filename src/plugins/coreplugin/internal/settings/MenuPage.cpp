// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "MenuPage.h"

#include <functional>

#include <QDebug>
#include <QQmlComponent>
#include <QSet>
#include <QSortFilterProxyModel>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionlayoutsmodel.h>
#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ActionCatalogProxyModel.h>
#include <coreplugin/internal/ActionConfiguration.h>

namespace Core::Internal {

    class LayoutFilterProxyModel : public QSortFilterProxyModel {
    public:
        explicit LayoutFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
            setRecursiveFilteringEnabled(true);
            setAutoAcceptChildRows(true);
        }
        void setSearchText(const QString &text) {
            m_text = text.trimmed();
            invalidateFilter();
        }
    protected:
        bool filterAcceptsRow(int row, const QModelIndex &parent) const override {
            if (m_text.isEmpty()) {
                return true;
            }
            const auto id = sourceModel()->index(row, 0, parent).data().toString();
            const auto info = CoreInterface::actionRegistry()->actionInfo(id);
            return id.contains(m_text, Qt::CaseInsensitive) ||
                   (info && (info->text().withoutMnemonic().contains(m_text, Qt::CaseInsensitive) ||
                             info->shortText().withoutMnemonic().contains(m_text, Qt::CaseInsensitive) ||
                             info->description().toString().contains(m_text, Qt::CaseInsensitive)));
        }
    private:
        QString m_text;
    };

    static bool isContainer(const QAK::ActionLayoutEntry &entry) {
        return entry.type() == QAK::ActionLayoutEntry::Menu || entry.type() == QAK::ActionLayoutEntry::Group;
    }

    static bool hasCycle(const QMap<QString, QVector<QAK::ActionLayoutEntry>> &map) {
        QSet<QString> visited;
        QSet<QString> active;
        std::function<bool(const QString &)> visit = [&](const QString &id) {
            if (active.contains(id)) {
                return true;
            }
            if (visited.contains(id)) {
                return false;
            }
            active.insert(id);
            for (const auto &entry : map.value(id)) {
                if (isContainer(entry) && visit(entry.id())) {
                    return true;
                }
            }
            active.remove(id);
            visited.insert(id);
            return false;
        };
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (visit(it.key())) {
                return true;
            }
        }
        return false;
    }

    MenuPage::MenuPage(QObject *parent) : ISettingPage("org.diffscope.core.Menu", parent) {
        setTitle(tr("Menus and Toolbars"));
        setDescription(tr("Configure the layout of menus and toolbars"));
        m_actionLayoutsModel = new QAK::ActionLayoutsModel(this);
        m_actionLayoutsModel->setRegistry(CoreInterface::actionRegistry());
        m_proxy = new LayoutFilterProxyModel(this);
        m_proxy->setSourceModel(m_actionLayoutsModel);
        m_catalogModel = new ActionCatalogProxyModel(this);
    }

    MenuPage::~MenuPage() {
        delete m_widget;
    }

    bool MenuPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString MenuPage::sortKeyword() const {
        return QStringLiteral("Menus");
    }

    QObject *MenuPage::widget() {
        if (!m_widget) {
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "MenuPage");
            m_widget = component.createWithInitialProperties({
                {"pageHandle", QVariant::fromValue(this)},
                {"model", QVariant::fromValue<QAbstractItemModel *>(m_proxy)},
            });
            if (!m_widget) {
                qFatal() << component.errorString();
            }
            m_widget->setParent(this);
        }
        return m_widget;
    }

    void MenuPage::beginSetting() {
        widget();
        QVector<QAK::ActionLayoutEntry> roots;
        auto registry = CoreInterface::actionRegistry();
        for (const auto &id : registry->actionIds()) {
            if (const auto info = registry->actionInfo(id); info && info->topLevel()) {
                roots.append(QAK::ActionLayoutEntry(id, info->type() == QAK::ActionItemInfo::Group ? QAK::ActionLayoutEntry::Group : QAK::ActionLayoutEntry::Menu));
            }
        }
        m_actionLayoutsModel->setTopLevelNodes(roots);
        m_actionLayoutsModel->setActionLayouts(registry->layouts());
        m_catalogModel->refresh();
        m_currentPath.clear();
        m_errorText.clear();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
        Q_EMIT stateChanged();
    }

    bool MenuPage::accept() {
        if (!ActionConfiguration::get()->saveLayouts(m_actionLayoutsModel->actionLayouts())) {
            m_errorText = tr("The menu and toolbar settings could not be saved.");
            Q_EMIT stateChanged();
            return false;
        }
        return ISettingPage::accept();
    }

    void MenuPage::endSetting() {
        if (m_widget) {
            m_widget->setProperty("started", false);
        }
        m_currentPath.clear();
        ISettingPage::endSetting();
    }

    QAbstractItemModel *MenuPage::catalogModel() const {
        return m_catalogModel;
    }

    QModelIndex MenuPage::indexForPath(const QVariantList &path) const {
        QModelIndex index;
        for (const auto &row : path) {
            index = m_actionLayoutsModel->index(row.toInt(), 0, index);
            if (!index.isValid()) {
                return {};
            }
        }
        return index;
    }

    QModelIndex MenuPage::sourceIndex() const {
        return indexForPath(m_currentPath);
    }

    QModelIndex MenuPage::currentIndex() const {
        return m_proxy->mapFromSource(sourceIndex());
    }

    QVariantList MenuPage::pathForIndex(const QModelIndex &index) const {
        QVariantList path;
        for (auto source = m_proxy->mapToSource(index); source.isValid(); source = source.parent()) {
            path.prepend(source.row());
        }
        return path;
    }

    void MenuPage::selectIndex(const QModelIndex &index) {
        const auto path = pathForIndex(index);
        if (m_currentPath != path) {
            m_currentPath = path;
            Q_EMIT stateChanged();
        }
    }

    QVariantList MenuPage::expansionKey(const QModelIndex &index) const {
        QVariantList key;
        for (auto source = m_proxy->mapToSource(index); source.isValid(); source = source.parent()) {
            const auto entry = source.data(Qt::UserRole).value<QAK::ActionLayoutEntry>();
            int occurrence = 0;
            for (int row = 0; row < source.row(); ++row) {
                if (m_actionLayoutsModel->index(row, 0, source.parent()).data(Qt::UserRole).value<QAK::ActionLayoutEntry>() == entry) {
                    ++occurrence;
                }
            }
            key.prepend(QVariantMap{{"id", entry.id()}, {"type", entry.type()}, {"occurrence", occurrence}});
        }
        return key;
    }

    QModelIndex MenuPage::indexForExpansionKey(const QVariantList &key) const {
        QModelIndex parent;
        for (const auto &part : key) {
            const auto component = part.toMap();
            const QAK::ActionLayoutEntry entry(component.value("id").toString(), static_cast<QAK::ActionLayoutEntry::Type>(component.value("type").toInt()));
            int occurrence = 0;
            QModelIndex match;
            for (int row = 0; row < m_actionLayoutsModel->rowCount(parent); ++row) {
                const auto index = m_actionLayoutsModel->index(row, 0, parent);
                if (index.data(Qt::UserRole).value<QAK::ActionLayoutEntry>() == entry && occurrence++ == component.value("occurrence").toInt()) {
                    match = index;
                    break;
                }
            }
            if (!match.isValid()) {
                return {};
            }
            parent = match;
        }
        return m_proxy->mapFromSource(parent);
    }

    void MenuPage::setSearchText(const QString &text) {
        m_proxy->setSearchText(text);
        Q_EMIT stateChanged();
    }

    bool MenuPage::canAdd() const {
        return currentIndex().isValid();
    }

    bool MenuPage::canRemove() const {
        return m_currentPath.size() > 1 && currentIndex().isValid();
    }

    bool MenuPage::canMoveUp() const {
        return canRemove() && sourceIndex().row() > 0;
    }

    bool MenuPage::canMoveDown() const {
        const auto index = sourceIndex();
        return canRemove() && index.row() + 1 < m_actionLayoutsModel->rowCount(index.parent());
    }

    bool MenuPage::canChangeForm() const {
        const auto type = currentForm();
        return canRemove() && (type == QAK::ActionLayoutEntry::Menu || type == QAK::ActionLayoutEntry::Group || type == QAK::ActionLayoutEntry::Separator || type == QAK::ActionLayoutEntry::Stretch);
    }

    bool MenuPage::canRestoreCurrent() const {
        QAK::ActionLayouts layouts;
        return prepareRestore(layouts) && layouts.adjacencyMap() != m_actionLayoutsModel->actionLayouts().adjacencyMap() && !hasCycle(layouts.adjacencyMap());
    }

    int MenuPage::currentForm() const {
        return sourceIndex().data(Qt::UserRole).value<QAK::ActionLayoutEntry>().type();
    }

    QString MenuPage::errorText() const {
        return m_errorText;
    }

    QString MenuPage::typeText(const QString &id, int type) const {
        return ActionCatalogProxyModel::typeText(id, type);
    }

    void MenuPage::changed() {
        m_errorText.clear();
        markDirty();
        Q_EMIT stateChanged();
    }

    bool MenuPage::addEntry(const QString &id, int type) {
        auto index = sourceIndex();
        if (!index.isValid() || type < QAK::ActionLayoutEntry::Action || type > QAK::ActionLayoutEntry::Stretch) {
            return false;
        }
        QVariantList parentPath = m_currentPath;
        int row = m_actionLayoutsModel->rowCount(index);
        if (!isContainer(index.data(Qt::UserRole).value<QAK::ActionLayoutEntry>())) {
            row = index.row() + 1;
            parentPath.removeLast();
        }
        const auto snapshot = m_actionLayoutsModel->actionLayouts();
        if (!m_actionLayoutsModel->insertRows(row, 1, indexForPath(parentPath))) {
            return false;
        }
        const auto inserted = m_actionLayoutsModel->index(row, 0, indexForPath(parentPath));
        if (!m_actionLayoutsModel->setData(inserted, QVariant::fromValue(QAK::ActionLayoutEntry(id, static_cast<QAK::ActionLayoutEntry::Type>(type))), Qt::UserRole)) {
            m_actionLayoutsModel->setActionLayouts(snapshot);
            m_errorText = tr("This item cannot be added because it would create an invalid or recursive menu.");
            Q_EMIT stateChanged();
            return false;
        }
        m_currentPath = parentPath;
        m_currentPath.append(row);
        changed();
        return true;
    }

    bool MenuPage::removeEntry() {
        if (!canRemove()) {
            return false;
        }
        const auto index = sourceIndex();
        const int row = index.row();
        auto parentPath = m_currentPath;
        parentPath.removeLast();
        if (!m_actionLayoutsModel->removeRows(row, 1, index.parent())) {
            return false;
        }
        m_currentPath = parentPath;
        const int count = m_actionLayoutsModel->rowCount(indexForPath(parentPath));
        if (count > 0) {
            m_currentPath.append(qMin(row, count - 1));
        }
        changed();
        return true;
    }

    bool MenuPage::moveEntry(const QVariantList &path, QVariantList parentPath, int row) {
        const auto source = indexForPath(path);
        const auto destination = indexForPath(parentPath);
        if (path.size() <= 1 || !source.isValid() || !destination.isValid()) {
            return false;
        }
        auto sourceParentPath = path;
        sourceParentPath.removeLast();
        const bool sameContainer = source.parent().data().toString() == destination.data().toString();
        const int sourceRow = source.row();
        if (!m_actionLayoutsModel->moveRows(source.parent(), sourceRow, 1, destination, row)) {
            m_errorText = tr("This item cannot be moved to the selected position.");
            Q_EMIT stateChanged();
            return false;
        }
        if (!sameContainer && parentPath.size() > sourceParentPath.size()) {
            bool samePrefix = true;
            for (int i = 0; i < sourceParentPath.size(); ++i) {
                samePrefix = samePrefix && parentPath[i] == sourceParentPath[i];
            }
            if (samePrefix && parentPath[sourceParentPath.size()].toInt() > sourceRow) {
                parentPath[sourceParentPath.size()] = parentPath[sourceParentPath.size()].toInt() - 1;
            }
        }
        m_currentPath = parentPath;
        m_currentPath.append(sameContainer && row > sourceRow ? row - 1 : row);
        changed();
        return true;
    }

    bool MenuPage::moveCurrent(int direction) {
        if ((direction < 0 && !canMoveUp()) || (direction >= 0 && !canMoveDown())) {
            return false;
        }
        auto parentPath = m_currentPath;
        const int row = parentPath.takeLast().toInt();
        return moveEntry(m_currentPath, parentPath, direction < 0 ? row - 1 : row + 2);
    }

    bool MenuPage::dropEntry(const QVariantList &path, const QModelIndex &target, int placement) {
        auto targetPath = pathForIndex(target);
        const auto index = indexForPath(targetPath);
        if (!index.isValid()) {
            return false;
        }
        if (placement == 0 && isContainer(index.data(Qt::UserRole).value<QAK::ActionLayoutEntry>())) {
            return moveEntry(path, targetPath, m_actionLayoutsModel->rowCount(index));
        }
        const int row = index.row() + (placement < 0 ? 0 : 1);
        targetPath.removeLast();
        return moveEntry(path, targetPath, row);
    }

    bool MenuPage::changeForm(int type) {
        if (!canChangeForm() || type == currentForm()) {
            return false;
        }
        const auto index = sourceIndex();
        const auto entry = index.data(Qt::UserRole).value<QAK::ActionLayoutEntry>();
        const bool toContainer = type == QAK::ActionLayoutEntry::Menu || type == QAK::ActionLayoutEntry::Group;
        const bool toSpacing = type == QAK::ActionLayoutEntry::Separator || type == QAK::ActionLayoutEntry::Stretch;
        if (!(isContainer(entry) ? toContainer : toSpacing)) {
            return false;
        }
        if (!m_actionLayoutsModel->setData(index, QVariant::fromValue(QAK::ActionLayoutEntry(entry.id(), static_cast<QAK::ActionLayoutEntry::Type>(type))), Qt::UserRole)) {
            return false;
        }
        changed();
        return true;
    }

    bool MenuPage::prepareRestore(QAK::ActionLayouts &layouts) const {
        const auto index = sourceIndex();
        if (!currentIndex().isValid() || !isContainer(index.data(Qt::UserRole).value<QAK::ActionLayoutEntry>())) {
            return false;
        }
        const auto defaults = CoreInterface::actionRegistry()->defaultLayouts().adjacencyMap();
        auto map = m_actionLayoutsModel->actionLayouts().adjacencyMap();
        QSet<QString> visited;
        std::function<void(const QString &)> restore = [&](const QString &id) {
            if (visited.contains(id)) {
                return;
            }
            visited.insert(id);
            if (defaults.contains(id)) {
                map[id] = defaults.value(id);
            } else {
                map.remove(id);
            }
            for (const auto &entry : defaults.value(id)) {
                if (isContainer(entry)) {
                    restore(entry.id());
                }
            }
        };
        restore(index.data().toString());
        layouts = QAK::ActionLayouts(map);
        return true;
    }

    bool MenuPage::restoreCurrent() {
        QAK::ActionLayouts layouts;
        if (!prepareRestore(layouts)) {
            return false;
        }
        if (hasCycle(layouts.adjacencyMap())) {
            m_errorText = tr("Restoring this menu would create a recursive menu. Restore all layouts or remove the conflicting reference.");
            Q_EMIT stateChanged();
            return false;
        }
        if (layouts.adjacencyMap() != m_actionLayoutsModel->actionLayouts().adjacencyMap()) {
            m_actionLayoutsModel->setActionLayouts(layouts);
            changed();
        }
        return true;
    }

    void MenuPage::restoreAll() {
        const auto defaults = CoreInterface::actionRegistry()->defaultLayouts();
        if (defaults.adjacencyMap() != m_actionLayoutsModel->actionLayouts().adjacencyMap()) {
            m_actionLayoutsModel->setActionLayouts(defaults);
            changed();
        }
    }

    bool MenuPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", Q_RETURN_ARG(bool, result), word);
        return result;
    }
}

#include "moc_MenuPage.cpp"
