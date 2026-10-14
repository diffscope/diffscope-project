// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ActionLayoutsEditorHelper_p.h"

#include <QJSValue>
#include <QQmlEngine>

#include <QAKCore/actionregistry.h>

namespace UIShell {

    ActionLayoutsEditorHelper::ActionLayoutsEditorHelper(QObject *parent)
        : QObject(parent), m_actionRegistry(nullptr) {
    }

    ActionLayoutsEditorHelper::~ActionLayoutsEditorHelper() = default;

    QAK::ActionRegistry *ActionLayoutsEditorHelper::actionRegistry() const {
        return m_actionRegistry;
    }

    void ActionLayoutsEditorHelper::setActionRegistry(QAK::ActionRegistry *actionRegistry) {
        if (m_actionRegistry == actionRegistry)
            return;

        m_actionRegistry = actionRegistry;
        emit actionRegistryChanged();
    }
    QJSValue ActionLayoutsEditorHelper::getActionDisplayInfo(const QString &id, const QAK::ActionLayoutEntry &entry) const {
        auto engine = qmlEngine(this);

        auto info = m_actionRegistry->actionInfo(id).value_or(QAK::ActionItemInfo());
        auto text = info.shortText().withoutMnemonic();
        if (text.isEmpty()) {
            text = id;
        }
        auto type = entry.type();
        auto actionIcon = m_actionRegistry->actionIcon("", id, info.icon()).value_or(QAK::ActionIcon());
        auto iconSource = actionIcon.url();
        auto iconColor = QColor::fromString(actionIcon.currentColor());

        auto ret = engine->newObject();
        ret.setProperty("text", text);
        ret.setProperty("id", id);
        ret.setProperty("iconSource", engine->fromVariant<QJSValue>(iconSource));
        ret.setProperty("iconColor", engine->fromVariant<QJSValue>(iconColor));
        ret.setProperty("type", type);
        ret.setProperty("topLevel", info.topLevel());
        return ret;
    }

    QString ActionLayoutsEditorHelper::highlightString(const QString &s, const QString &t, const QColor &c) {
        if (t.isEmpty()) {
            return s.toHtmlEscaped();
        }
        QString result;
        qsizetype pos = 0;
        auto matchPos = s.indexOf(t, pos, Qt::CaseInsensitive);
        while (matchPos != -1) {
            result += s.mid(pos, matchPos - pos).toHtmlEscaped();
            result += QStringLiteral("<span style='background-color: rgba(%1, %2, %3, %4);'>").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alphaF());
            result += s.mid(matchPos, t.length()).toHtmlEscaped();
            result += QStringLiteral("</span>");
            pos = matchPos + t.length();
            matchPos = s.indexOf(t, pos, Qt::CaseInsensitive);
        }
        result += s.mid(pos).toHtmlEscaped();
        return result;
    }

}

#include "moc_ActionLayoutsEditorHelper_p.cpp"
