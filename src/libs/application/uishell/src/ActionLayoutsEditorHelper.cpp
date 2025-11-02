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

        auto info = m_actionRegistry->actionInfo(id);
        auto text = info.text(true).isEmpty() ? info.text(false) : info.text(true);
        auto type = entry.type();
        auto actionIcon = m_actionRegistry->actionIcon("", id);
        auto iconSource = QUrl::fromLocalFile(actionIcon.filePath());
        auto iconColor = QColor::fromString(actionIcon.currentColor());

        auto ret = engine->newObject();
        ret.setProperty("text", text);
        ret.setProperty("iconSource", engine->fromVariant<QJSValue>(iconSource));
        ret.setProperty("iconColor", engine->fromVariant<QJSValue>(iconColor));
        ret.setProperty("type", type);
        ret.setProperty("topLevel", info.topLevel());
        return ret;
    }

    static QString removeMnemonic(const QString &s) {
        QString text(s.size(), QChar::Null);
        int idx = 0;
        int pos = 0;
        int len = s.size();
        while (len) {
            if (s.at(pos) == QLatin1Char('&') && (len == 1 || s.at(pos + 1) != QLatin1Char('&'))) {
                ++pos;
                --len;
                if (len == 0)
                    break;
            } else if (s.at(pos) == QLatin1Char('(') && len >= 4 && s.at(pos + 1) == QLatin1Char('&') && s.at(pos + 2) != QLatin1Char('&') && s.at(pos + 3) == QLatin1Char(')')) {
                // a mnemonic with format "\s*(&X)"
                int n = 0;
                while (idx > n && text.at(idx - n - 1).isSpace())
                    ++n;
                idx -= n;
                pos += 4;
                len -= 4;
                continue;
            }
            text[idx] = s.at(pos);
            ++pos;
            ++idx;
            --len;
        }
        text.truncate(idx);
        return text;
    }

    QString ActionLayoutsEditorHelper::highlightString(const QString &s_, const QString &t, const QColor &c) {
        QString s = removeMnemonic(s_);
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
            matchPos = s.indexOf(t, pos);
        }
        result += s.mid(pos).toHtmlEscaped();
        return result;
    }

}

#include "moc_ActionLayoutsEditorHelper_p.cpp"
