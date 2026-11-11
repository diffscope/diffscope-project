// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspaceSession.h"

#include <algorithm>
#include <memory>
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
#include <coreplugin/internal/WorkspacePanelCatalog.h>
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
        : QObject(parent), m_window(window), m_context(new SVS::DockingContext(this)), m_embeddedHost(new SVS::DockingHost(this)), m_loader(new WorkspacePanelLoader(window, this)) {
        m_embeddedHost->setHostId("main");
        m_embeddedHost->setWindow(qobject_cast<QQuickWindow *>(window->window()));
        m_embeddedHost->setContext(m_context);

        connect(window, &ExecutiveInterface::loadingStateChanged, this, [this](ExecutiveInterface::State state) {
            if (state >= ExecutiveInterface::Exiting)
                shutdown();
        });

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
            ensureContent(request->panel(), false, [this, guard](bool ready) {
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

    void WorkspaceSession::initialize() {
        auto catalog = RuntimeInterface::instance()->getFirstObject<WorkspacePanelCatalog>();
        if (catalog) {
            for (const auto &entry : catalog->entries()) {
                auto map = entry.toMap();
                auto panel = ensurePanel(map.value("id").toString());
                panel->setTitle(map.value("text").toString());
                panel->setIconSource(map.value("iconSource").toUrl());
                panel->setIconColor(map.value("iconColor").value<QColor>());
            }
        }
    }

    SVS::DockingPanel *WorkspaceSession::ensurePanel(const QString &id) {
        if (auto p = m_context->panel(id))
            return p;

        auto p = new SVS::DockingPanel(this);
        p->setKey(id);
        p->setTitle(id);
        connect(p, &SVS::DockingPanel::placementChanged, this, [this, p, previousWindow = QPointer<QQuickWindow>{}]() mutable {
            auto window = floatingWindowForPanel(p);
            if (previousWindow && previousWindow.data() != window)
                m_panelFloatingGeometries.insert(p->key(), floatingWindowGeometry(previousWindow));
            previousWindow = window;
        });
        m_context->registerPanel(p);
        return p;
    }

    void WorkspaceSession::ensureContent(SVS::DockingPanel *panel, bool placeholder, std::function<void(bool)> completion) {
        auto id = panel->key();
        if (m_contents.value(id)) {
            completion(true);
            return;
        }
        if (m_pending.contains(id)) {
            m_pending[id].append(std::move(completion));
            return;
        }

        m_pending.insert(id, {std::move(completion)});
        m_loader->load(panel, placeholder, [this, id](QQuickItem *item, const QString &error) {
            if (m_shuttingDown)
                return;
            if (item) {
                m_contents.insert(id, item);
                if (!m_busy)
                    WorkspacePanelLoader::loadState(item, m_panelStates.value(id));
            }
            if (!error.isEmpty())
                Q_EMIT errorOccurred(error);

            auto callbacks = m_pending.take(id);
            for (const auto &callback : callbacks)
                callback(bool(item));
        });
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

        m_busy = true;
        Q_EMIT busyChanged();

        auto state = layout.state();
        QSet<QString> ids;
        std::function<void(const SVS::DockingNodeState &)> collect = [&](const auto &node) {
            for (const auto &side : node.panelIds) {
                for (const auto &id : side)
                    ids.insert(id);
            }
            for (const auto &child : node.children)
                collect(child);
        };
        auto root = SVS::DockingLayoutState::fromVariant(state.value("mainRoot").toMap());
        if (root.hasRoot)
            collect(root.root);
        for (const auto &f : state.value("floatingHosts").toList())
            collect(SVS::DockingLayoutState::fromVariant(f.toMap().value("root").toMap()).root);

        auto remaining = std::make_shared<int>(ids.size());
        auto success = std::make_shared<bool>(true);
        auto finish = [this, state, success]() {
            if (m_shuttingDown)
                return;

            QList<SVS::DockingHost *> preparedHosts;
            QList<SVS::DockingHost *> targetHosts;
            const auto floatingHosts = state.value("floatingHosts").toList();
            const auto activePanelId = state.value("activePanelId").toString();
            if (*success) {
                for (const auto &value : floatingHosts) {
                    auto host = m_context->host(value.toMap().value("hostId").toString());
                    if (!host) {
                        host = createFloatingHost(value.toMap(), true);
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
                if (!m_context->restoreLayouts(roots, activePanelId).accepted()) {
                    Q_EMIT errorOccurred(tr("The workspace layout could not be restored."));
                    *success = false;
                }
            }

            if (*success) {
                m_panelFloatingGeometries.clear();
                const auto savedGeometries = state.value("panelFloatingGeometries").toMap();
                for (auto it = savedGeometries.cbegin(); it != savedGeometries.cend(); ++it)
                    if (it.value().toMap().value("normalGeometry").toRect().isValid())
                        m_panelFloatingGeometries.insert(it.key(), it.value());

                m_panelStates = state.value("panelStates").toMap();
                for (auto it = m_contents.cbegin(); it != m_contents.cend(); ++it)
                    WorkspacePanelLoader::loadState(it.value(), m_panelStates.value(it.key()));

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
            m_busy = false;
            if (!m_ready) {
                m_ready = true;
                Q_EMIT readyChanged();
            }
            Q_EMIT busyChanged();
            Q_EMIT restored();

            if (m_pendingLayout.isValid()) {
                auto next = m_pendingLayout;
                m_pendingLayout = {};
                QTimer::singleShot(0, this, [this, next] { apply(next); });
            }
        };
        if (ids.isEmpty()) {
            finish();
            return;
        }
        for (const auto &id : ids)
            ensureContent(ensurePanel(id), true, [remaining, success, finish](bool ready) {
                *success = *success && ready;
                if (--*remaining == 0)
                    finish();
            });
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
        if (m_pending.contains(request->panel()->key())) {
            request->reject("contentPending");
            return;
        }
        ensureContent(request->panel(), false, [this, guard](bool ready) {
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
        for (auto it = m_contents.cbegin(); it != m_contents.cend(); ++it) {
            QString error;
            auto value = WorkspacePanelLoader::saveState(it.value(), &error);
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
        m_loader->cancel();
        Q_EMIT shuttingDownChanged();
        m_context->cancelDrag();
        m_pending.clear();
        m_pendingLayout = {};
        disposeFloatingHosts();
        m_embeddedHost->layout()->clear();
    }
}

#include "moc_WorkspaceSession.cpp"

