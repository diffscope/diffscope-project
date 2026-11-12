// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspaceSession.h"

#include <algorithm>
#include <utility>

#include <QDebug>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QVariantList>

#include <CoreApi/executiveinterface.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/DockingContext.h>
#include <SVSCraftQuick/DockingGroup.h>
#include <SVSCraftQuick/DockingHost.h>
#include <SVSCraftQuick/DockingLayout.h>
#include <SVSCraftQuick/DockingPanel.h>
#include <SVSCraftQuick/DockingRequest.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/WorkspacePanelRegistry.h>
#include <coreplugin/internal/WorkspacePanelLoader.h>

namespace Core::Internal {
    static QVariantMap floatingWindowGeometry(const QQuickWindow *window) {
        auto normalGeometry = window->windowState() == Qt::WindowNoState ? window->geometry() : window->property("savedNormalGeometry").toRect();
        if (!normalGeometry.isValid())
            normalGeometry = window->geometry();
        const auto windowState = window->windowStates().testFlag(Qt::WindowMaximized) ? Qt::WindowMaximized : Qt::WindowNoState;
        return {{"normalGeometry", normalGeometry}, {"windowState", int(windowState)}, {"screenName", window->screen() ? window->screen()->name() : QString{}}};
    }

    WorkspaceSession::WorkspaceSession(ProjectWindowInterface *window, QObject *parent)
        : QObject(parent), m_window(window), m_registry(window->workspacePanelRegistry()), m_context(new SVS::DockingContext(this)), m_embeddedHost(new SVS::DockingHost(this)) {
        m_embeddedHost->setHostId("main");
        m_embeddedHost->setWindow(qobject_cast<QQuickWindow *>(window->window()));
        m_embeddedHost->setContext(m_context);

        connect(window, &ExecutiveInterface::loadingStateChanged, this, [this](ExecutiveInterface::State state) {
            if (state >= ExecutiveInterface::Exiting)
                shutdown();
        });
        connect(m_registry.data(), &WorkspacePanelRegistry::registrationChanged, this, [this](const QString &id) {
            auto &versions = m_panelVersions[id];
            ++versions.registration;
            ++versions.component;
            schedulePanelUpdate(id);
        });
        connect(m_registry.data(), &WorkspacePanelRegistry::componentStatusChanged, this, [this](const QString &id) {
            ++m_panelVersions[id].component;
            schedulePanelUpdate(id);
        });
        for (const auto &id : m_registry->panelIds())
            schedulePanelUpdate(id);

        connect(m_context, &SVS::DockingContext::placementRequested, this, [this](SVS::DockingRequest *request) {
            QPointer<SVS::DockingRequest> guard(request);
            if (!m_ready) {
                request->reject("workspaceNotReady");
                return;
            }
            if (m_busy || m_shuttingDown) {
                request->reject("workspaceBusy");
                return;
            }
            requestContent(request->panel(), [this, guard](bool ready) {
                if (!guard)
                    return;
                if (!ready || m_busy || m_shuttingDown)
                    guard->reject("contentUnavailable");
                else if (!guard->accept().accepted())
                    Q_EMIT errorOccurred(tr("The panel could not be placed in the selected location."));
            });
        });
        connect(m_context, &SVS::DockingContext::floatingRequested, this, &WorkspaceSession::floatRequest);
    }

    WorkspaceSession::~WorkspaceSession() {
        shutdown();
    }

    SVS::DockingContext *WorkspaceSession::context() const {
        return m_context;
    }

    SVS::DockingHost *WorkspaceSession::embeddedHost() const {
        return m_embeddedHost;
    }

    bool WorkspaceSession::isBusy() const {
        return m_busy;
    }

    bool WorkspaceSession::isReady() const {
        return m_ready;
    }

    bool WorkspaceSession::isShuttingDown() const {
        return m_shuttingDown;
    }

    SVS::DockingPanel *WorkspaceSession::ensurePanel(const QString &id) {
        if (auto p = m_context->panel(id))
            return p;

        auto p = new SVS::DockingPanel(this);
        p->setKey(id);
        p->setTitle(id);
        connect(p, &SVS::DockingPanel::placementChanged, this, [this, p, previousWindow = QPointer<QQuickWindow>{}]() mutable {
            if (m_shuttingDown)
                return;
            auto window = floatingWindowForPanel(p);
            if (previousWindow && previousWindow.data() != window)
                m_panelFloatingGeometries.insert(p->key(), floatingWindowGeometry(previousWindow));
            previousWindow = window;
            if (m_registry && p->context() && !p->group() && (!m_registry->component(p->key()) || m_contents.value(p->key()).fallback))
                schedulePanelUpdate(p->key());
        });
        m_context->registerPanel(p);
        return p;
    }

    void WorkspaceSession::schedulePanelUpdate(const QString &id) {
        if (m_shuttingDown || !m_registry)
            return;
        m_panelUpdates.insert(id);
        schedulePanelUpdates();
    }

    void WorkspaceSession::schedulePanelUpdates() {
        if (m_panelUpdatePending || m_shuttingDown || !m_registry)
            return;
        m_panelUpdatePending = true;
        QMetaObject::invokeMethod(this, &WorkspaceSession::updatePanels, Qt::QueuedConnection);
    }

    void WorkspaceSession::updatePanels() {
        if (m_shuttingDown || !m_registry)
            return;

        const auto updates = std::exchange(m_panelUpdates, {});
        for (const auto &id : updates) {
            if (m_shuttingDown || !m_registry)
                break;
            updatePanel(id);
        }
        finishLayoutRestore();
        m_panelUpdatePending = false;
        if (m_shuttingDown || !m_registry)
            return;
        if (!m_panelUpdates.isEmpty() || (m_busy && m_restoringPanelIds.isEmpty()))
            schedulePanelUpdates();
        Q_EMIT panelsChanged();
    }

    void WorkspaceSession::updatePanel(const QString &id) {
        const auto versions = m_panelVersions.value(id);
        const QPointer<QQmlComponent> component = m_registry->component(id);
        const bool restoring = m_restoringPanelIds.contains(id);
        auto panel = m_context->panel(id);
        if (!panel && (component || restoring))
            panel = ensurePanel(id);
        if (!panel) {
            finishPanelRequests(id, false);
            return;
        }

        const auto current = [this, &id, &versions, &component] {
            return !m_shuttingDown && m_registry && m_panelVersions.value(id).component == versions.component && m_registry->component(id) == component.data();
        };
        if (!current())
            return;

        const bool placed = panel->group();
        const bool waiting = component && component->isLoading();
        const bool allowFallback = placed || restoring;
        auto content = m_contents.value(id);
        const bool keepFallback = content.fallback && allowFallback && (waiting || content.version == versions.component);
        bool restoreFocus = false;
        if (content.item && !keepFallback && (content.fallback || content.version != versions.component)) {
            const auto window = content.item->window();
            const auto focus = window && window->isActive() ? window->activeFocusItem() : nullptr;
            restoreFocus = focus && (focus == content.item || content.item->isAncestorOf(focus));
            unloadContent(panel);
            content = {};
            if (!current())
                return;
        }

        const auto metadata = m_registry->metadata(id);
        const auto title = metadata.title.isEmpty() ? m_panelTitles.value(id, id) : metadata.title;
        if (!content.item) {
            panel->setTitle(title);
            panel->setDescription(metadata.description);
            panel->setIconSource(metadata.iconSource);
            panel->setIconColor(metadata.iconColor);
            if (!current())
                return;
        }

        const bool requested = std::ranges::any_of(m_panelRequests.value(id), [&](const auto &request) {
            return request.registration == versions.registration;
        });
        if (!content.item && (allowFallback || requested) && (!waiting || placed)) {
            QString error;
            QPointer<QQuickItem> item;
            if (!waiting)
                item = WorkspacePanelLoader::create(component, panel, &error);
            if (item && current())
                Q_EMIT panelContentCreated(id, item);
            if (!current()) {
                delete item.data();
                return;
            }

            const bool fallback = !item;
            if (fallback && allowFallback)
                item = WorkspacePanelLoader::createFallback(panel, title, bool(component), error);
            if (item) {
                item->setParent(this);
                QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
                item->setVisible(false);
                if (!current()) {
                    delete item.data();
                    return;
                }
                m_contents.insert(id, {item, versions.component, fallback, !fallback});
                panel->setContentItem(item);
                if (item && current())
                    panel->setHeaderItem(item->property("header").value<QQuickItem *>());
                if (item && !fallback && current()) {
                    if (panel->title().isEmpty())
                        panel->setTitle(title);
                    if (panel->description().isEmpty())
                        panel->setDescription(metadata.description);
                    if (panel->iconSource().isEmpty() && !metadata.iconSource.isEmpty()) {
                        panel->setIconSource(metadata.iconSource);
                        if (metadata.iconColor.isValid())
                            panel->setIconColor(metadata.iconColor);
                    }
                    if (item && current() && !m_busy) {
                        m_contents[id].statePending = false;
                        WorkspacePanelLoader::loadState(item, m_panelStates.value(id));
                    }
                }
                if (restoreFocus && item && current() && !m_busy && panel->isShown())
                    item->forceActiveFocus();
            }
            if (current() && !error.isEmpty() && (component || restoring || requested))
                Q_EMIT errorOccurred(error);
        }

        if (!current())
            return;
        finishPanelRequests(id, waiting);
        if (current() && !component && !panel->group() && !m_restoringPanelIds.contains(id)) {
            if (m_context->draggedPanelKey() == id)
                m_context->cancelDrag();
            m_context->unregisterPanel(panel);
            panel->deleteLater();
        }
    }

    void WorkspaceSession::unloadContent(SVS::DockingPanel *panel) {
        const auto id = panel->key();
        const auto content = m_contents.take(id);
        if (!content.item)
            return;

        QString error;
        if (!content.fallback) {
            if (!panel->title().isEmpty())
                m_panelTitles.insert(id, panel->title());
            if (!content.statePending) {
                const auto state = WorkspacePanelLoader::saveState(content.item, &error);
                if (error.isEmpty() && state.isValid())
                    m_panelStates.insert(id, state);
            }
        }

        if (m_context->draggedPanelKey() == id)
            m_context->cancelDrag();
        // Detaches the items before destruction to preserve the docking position.
        panel->setContentItem(nullptr);
        panel->setHeaderItem(nullptr);
        if (content.item)
            content.item->deleteLater();
        if (!error.isEmpty())
            Q_EMIT errorOccurred(error);
    }

    void WorkspaceSession::requestContent(SVS::DockingPanel *panel, std::function<void(bool)> completion) {
        const auto id = panel->key();
        const auto versions = m_panelVersions.value(id);
        const auto content = m_contents.value(id);
        if (content.item && content.version == versions.component && (!content.fallback || panel->group())) {
            completion(true);
            return;
        }
        m_panelRequests[id].append({versions.registration, std::move(completion)});
        schedulePanelUpdate(id);
    }

    void WorkspaceSession::finishPanelRequests(const QString &id, bool waiting) {
        auto requests = m_panelRequests.take(id);
        for (auto &request : requests) {
            if (m_shuttingDown)
                return;

            const auto versions = m_panelVersions.value(id);
            if (request.registration != versions.registration || m_busy) {
                request.completion(false);
            } else if (waiting || m_panelUpdates.contains(id)) {
                m_panelRequests[id].append(std::move(request));
            } else {
                const auto content = m_contents.value(id);
                const auto panel = m_context->panel(id);
                request.completion(content.item && content.version == versions.component && (!content.fallback || (panel && panel->group())));
            }
        }
    }

    void WorkspaceSession::apply(const ProjectWindowWorkspaceLayout &layout) {
        if (m_shuttingDown)
            return;
        if (!layout.isValid()) {
            Q_EMIT errorOccurred(tr("The workspace layout is invalid."));
            return;
        }
        if (m_busy) {
            m_pendingLayout = layout;
            return;
        }

        m_restoringLayout = layout;
        const auto state = layout.state();
        std::function<void(const SVS::DockingNodeState &)> collect = [&](const auto &node) {
            for (const auto &side : node.panelIds) {
                for (const auto &id : side)
                    m_restoringPanelIds.insert(id);
            }
            for (const auto &child : node.children)
                collect(child);
        };
        auto root = SVS::DockingLayoutState::fromVariant(state.value("mainRoot").toMap());
        if (root.hasRoot)
            collect(root.root);
        for (const auto &f : state.value("floatingHosts").toList())
            collect(SVS::DockingLayoutState::fromVariant(f.toMap().value("root").toMap()).root);

        m_busy = true;
        m_panelUpdates.unite(m_restoringPanelIds);
        schedulePanelUpdates();
        Q_EMIT busyChanged();
    }

    void WorkspaceSession::finishLayoutRestore() {
        if (!m_busy || m_shuttingDown || !m_registry)
            return;
        for (const auto &id : std::as_const(m_restoringPanelIds)) {
            const auto content = m_contents.value(id);
            const auto component = m_registry->component(id);
            if (!content.item || content.version != m_panelVersions.value(id).component || (component && component->isLoading()))
                return;
        }

        const auto state = m_restoringLayout.state();
        const auto floatingHosts = state.value("floatingHosts").toList();
        const auto activePanelId = state.value("activePanelId").toString();
        QList<SVS::DockingHost *> preparedHosts;
        QList<SVS::DockingHost *> targetHosts;
        for (const auto &value : floatingHosts) {
            const auto spec = value.toMap();
            auto host = m_context->host(spec.value("hostId").toString());
            if (!host) {
                host = createFloatingHost(spec, true);
                preparedHosts.append(host);
            }
            targetHosts.append(host);
        }
        for (auto host : preparedHosts)
            host->setContext(m_context);

        QVariantMap roots;
        for (auto host : m_context->hosts())
            roots.insert(host->hostId(), QVariantMap{});
        roots.insert(m_embeddedHost->hostId(), state.value("mainRoot"));
        for (const auto &value : floatingHosts) {
            const auto spec = value.toMap();
            roots.insert(spec.value("hostId").toString(), spec.value("root"));
        }
        const bool restoredLayout = m_context->restoreLayouts(roots, activePanelId).accepted();
        if (restoredLayout) {
            m_panelFloatingGeometries.clear();
            const auto savedGeometries = state.value("panelFloatingGeometries").toMap();
            for (auto it = savedGeometries.cbegin(); it != savedGeometries.cend(); ++it)
                if (it.value().toMap().value("normalGeometry").toRect().isValid())
                    m_panelFloatingGeometries.insert(it.key(), it.value());
            m_panelStates = state.value("panelStates").toMap();
            for (auto &content : m_contents)
                content.statePending = !content.fallback;
        } else {
            Q_EMIT errorOccurred(tr("The workspace layout could not be restored."));
        }

        const auto contents = m_contents;
        for (auto it = contents.cbegin(); it != contents.cend(); ++it) {
            if (m_shuttingDown)
                return;
            if (it->item && it->statePending && it->version == m_panelVersions.value(it.key()).component) {
                m_contents[it.key()].statePending = false;
                WorkspacePanelLoader::loadState(it->item, m_panelStates.value(it.key()));
            }
        }

        if (restoredLayout) {
            for (auto host : m_context->hosts())
                if (host != m_embeddedHost && !targetHosts.contains(host)) {
                    if (auto window = m_floatingWindows.take(host)) {
                        window->hide();
                        window->deleteLater();
                    }
                    m_context->unregisterHost(host, true);
                    host->deleteLater();
                }
            for (qsizetype i = 0; i < targetHosts.size(); ++i) {
                if (auto window = m_floatingWindows.value(targetHosts[i])) {
                    restoreWindowGeometry(window, floatingHosts[i].toMap());
                    window->setVisible(true);
                }
            }
            auto active = m_context->panel(activePanelId);
            if (active && active->isShown())
                m_context->activate(active);
        } else {
            for (auto host : preparedHosts) {
                if (auto window = m_floatingWindows.take(host))
                    delete window;
                delete host;
            }
        }

        const auto restoredIds = std::exchange(m_restoringPanelIds, {});
        m_restoringLayout = {};
        for (const auto &id : restoredIds) {
            const auto panel = m_context->panel(id);
            if (panel && !panel->group() && m_contents.value(id).fallback)
                schedulePanelUpdate(id);
        }

        m_busy = false;
        if (!m_ready) {
            m_ready = true;
            Q_EMIT readyChanged();
        }
        Q_EMIT busyChanged();
        Q_EMIT restored();

        if (m_pendingLayout.isValid()) {
            const auto next = std::exchange(m_pendingLayout, {});
            QTimer::singleShot(0, this, [this, next] { apply(next); });
        }
    }

    QQuickWindow *WorkspaceSession::floatingWindowForPanel(const SVS::DockingPanel *panel) const {
        const auto group = panel->group();
        return group ? m_floatingWindows.value(group->layout()->host()).data() : nullptr;
    }

    SVS::DockingHost *WorkspaceSession::createFloatingHost(const QVariantMap &spec, bool deferRegistration) {
        auto host = new SVS::DockingHost(this);
        auto id = spec.value("hostId").toString();
        if (!id.isEmpty())
            host->setHostId(id);
        host->setRole(SVS::DockingHost::Floating);
        if (!deferRegistration)
            host->setContext(m_context);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "DockingWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"host", QVariant::fromValue(host)}});
        if (!object) {
            qFatal() << component.errorString();
        }
        auto window = qobject_cast<QQuickWindow *>(object);
        if (!window) {
            qFatal("DockingWindow in SVSCraft.UIComponents is not QQuickWindow");
        }
#ifdef Q_OS_MACOS
        window->setFlags((window->flags() & ~Qt::WindowType_Mask) | Qt::Tool);
#endif
        window->QObject::setParent(this);
        QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
        window->setProperty("interactionEnabled", !m_busy);
        connect(this, &WorkspaceSession::busyChanged, window, [this, window] { window->setProperty("interactionEnabled", !m_busy); });
        window->setTransientParent(m_window->window());
        restoreWindowGeometry(window, spec);
        m_floatingWindows.insert(host, window);
        connect(
            host, &SVS::DockingHost::closeRequested, this,
            [this, guard = QPointer(host)] {
                auto host = guard.data();
                if (!host || !m_floatingWindows.contains(host) || m_busy || m_shuttingDown || !host->isEmpty())
                    return;

                if (auto window = m_floatingWindows.take(host))
                    window->deleteLater();
                m_context->unregisterHost(host, true);
                host->deleteLater();
            },
            Qt::QueuedConnection
        );
        return host;
    }

    void WorkspaceSession::restoreWindowGeometry(QQuickWindow *window, const QVariantMap &spec) {
        auto rect = spec.value("normalGeometry").toRect();
        if (!rect.isValid())
            rect = QRect(m_window->window()->position() + QPoint(40, 40), QSize(500, 400));
        auto screens = QGuiApplication::screens();
        auto it = std::find_if(screens.begin(), screens.end(), [&](auto screen) { return screen->name() == spec.value("screenName").toString(); });
        auto screen = it == screens.end() ? m_window->window()->screen() : *it;
        if (screen) {
            window->setScreen(screen);
            auto bounds = screen->availableGeometry();
            rect.setSize(rect.size().boundedTo(bounds.size()));
            rect.moveLeft(std::clamp(rect.x(), bounds.left(), bounds.right() - rect.width() + 1));
            rect.moveTop(std::clamp(rect.y(), bounds.top(), bounds.bottom() - rect.height() + 1));
        }
        window->setWindowState(Qt::WindowNoState);
        window->setGeometry(rect);
        window->setProperty("savedNormalGeometry", rect);
        if (spec.value("windowState").toInt() == Qt::WindowMaximized)
            window->setWindowState(Qt::WindowMaximized);
    }

    void WorkspaceSession::floatRequest(SVS::DockingRequest *request) {
        QPointer<SVS::DockingRequest> guard(request);
        if (!m_ready) {
            request->reject("workspaceNotReady");
            return;
        }
        if (m_busy || m_shuttingDown) {
            request->reject("workspaceBusy");
            return;
        }
        if (m_panelRequests.contains(request->panel()->key())) {
            request->reject("contentPending");
            return;
        }
        requestContent(request->panel(), [this, guard](bool ready) {
            if (!guard)
                return;
            if (!ready || m_busy || m_shuttingDown) {
                guard->reject("contentUnavailable");
                return;
            }
            auto panel = guard->panel();
            auto spec = m_panelFloatingGeometries.value(panel->key()).toMap();
            if (auto window = floatingWindowForPanel(panel))
                spec = floatingWindowGeometry(window);
            if (!spec.value("normalGeometry").toRect().isValid()) {
                auto hint = panel->effectivePreferredSize().expandedTo(panel->minimumSize());
                spec.insert("normalGeometry", QRect(m_window->window()->position() + QPoint(40, 40), QSize(std::max(400, int(hint.width())), std::max(300, int(hint.height()) + 64))));
            }
            auto host = createFloatingHost(spec);
            auto result = guard->accept(host);
            if (result.accepted()) {
                if (auto window = m_floatingWindows.value(host))
                    window->setVisible(true);
            } else {
                host->close();
                Q_EMIT errorOccurred(tr("The panel could not be moved to a floating window."));
            }
        });
    }

    ProjectWindowWorkspaceLayout WorkspaceSession::snapshot(const QString &name) const {
        auto states = m_panelStates;
        const auto contents = m_contents;
        for (auto it = contents.cbegin(); it != contents.cend(); ++it) {
            if (!it->item || it->fallback || it->statePending)
                continue;
            QString error;
            auto value = WorkspacePanelLoader::saveState(it->item, &error);
            if (error.isEmpty()) {
                if (value.isValid())
                    states.insert(it.key(), value);
            } else
                Q_EMIT const_cast<WorkspaceSession *>(this)->errorOccurred(error);
        }
        QVariantList floats;
        auto floatingGeometries = m_panelFloatingGeometries;
        for (auto host : m_context->hosts())
            if (host != m_embeddedHost && !host->isEmpty()) {
                auto window = m_floatingWindows.value(host);
                if (!window)
                    continue;
                const auto geometry = floatingWindowGeometry(window);
                for (auto group : host->layout()->groups())
                    for (auto side : {SVS::DockingGroup::Leading, SVS::DockingGroup::Trailing})
                        for (auto panel : group->panels(side))
                            floatingGeometries.insert(panel->key(), geometry);

                auto spec = geometry;
                spec.insert("hostId", host->hostId());
                spec.insert("root", host->layout()->save());
                spec.insert("savedAvailableGeometry", window->screen() ? window->screen()->availableGeometry() : QRect{});
                floats.append(spec);
            }

        ProjectWindowWorkspaceLayout layout;
        layout.setName(name);
        layout.setState({{"mainRoot", m_embeddedHost->layout()->save()}, {"floatingHosts", floats}, {"panelFloatingGeometries", floatingGeometries}, {"panelStates", states}, {"activePanelId", m_context->activePanel() ? m_context->activePanel()->key() : QString{}}});
        return layout;
    }

    void WorkspaceSession::disposeFloatingHosts() {
        for (auto host : m_context->hosts())
            if (host != m_embeddedHost) {
                if (auto window = m_floatingWindows.take(host)) {
                    window->hide();
                    window->deleteLater();
                }
                m_context->unregisterHost(host, true);
                host->deleteLater();
            }
    }

    void WorkspaceSession::shutdown() {
        if (m_shuttingDown)
            return;
        m_shuttingDown = true;
        Q_EMIT shuttingDownChanged();
        m_context->cancelDrag();
        m_panelRequests.clear();
        m_panelUpdates.clear();
        m_restoringPanelIds.clear();
        m_restoringLayout = {};
        m_pendingLayout = {};
        disposeFloatingHosts();
        m_embeddedHost->layout()->clear();
    }
}

#include "moc_WorkspaceSession.cpp"

