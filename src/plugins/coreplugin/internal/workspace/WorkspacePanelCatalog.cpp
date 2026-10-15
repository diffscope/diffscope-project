// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "WorkspacePanelCatalog.h"

#include <QColor>
#include <QSet>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>

namespace Core::Internal {
    WorkspacePanelCatalog::WorkspacePanelCatalog(QObject *parent) : QObject(parent) {}

    void WorkspacePanelCatalog::capture() {
        m_ids = CoreInterface::actionRegistry()->catalog().children("org.diffscope.core.workspacePanelWidgets");
        m_ids.removeDuplicates();
        m_ids.removeIf([](const QString &id) { return !CoreInterface::actionRegistry()->actionInfo(id).has_value(); });

        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        bool initialized = settings->value("hasPreviousSnapshot", false).toBool();
        auto previous = settings->value("registeredPanelIds").toStringList();
        if (initialized) {
            for (const auto &id : m_ids) {
                if (!previous.contains(id))
                    m_added.append(id);
            }
        }

        settings->setValue("hasPreviousSnapshot", true);
        settings->setValue("registeredPanelIds", m_ids);
        settings->endGroup();
    }

    QVariantList WorkspacePanelCatalog::entries() const {
        QVariantList result;
        for (const auto &id : m_ids) {
            auto info = CoreInterface::actionRegistry()->actionInfo(id).value_or(QAK::ActionItemInfo());
            auto icon = CoreInterface::actionRegistry()->actionIcon("", id, info.icon()).value_or(QAK::ActionIcon());
            result.append(QVariantMap{
                {"id", id},
                {"text", info.text().toString()},
                {"iconSource", icon.url()},
                {"iconColor", QColor::fromString(icon.currentColor())}
            });
        }
        return result;
    }

    QVariantList WorkspacePanelCatalog::pendingEntries() const {
        QVariantList result;
        if (!m_presented) {
            for (const auto &entry : entries()) {
                if (m_added.contains(entry.toMap().value("id").toString()))
                    result.append(entry);
            }
        }
        return result;
    }

    void WorkspacePanelCatalog::markPresented() {
        m_presented = true;
    }
}

#include "moc_WorkspacePanelCatalog.cpp"

