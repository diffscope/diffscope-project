#ifndef DIFFSCOPE_UTILS_ACTIONHELPER_H
#define DIFFSCOPE_UTILS_ACTIONHELPER_H

#include <QObject>
#include <QString>

namespace QAK {
    class QuickActionContext;
}

namespace Core::Internal {

    class ActionHelper {
    public:
        static bool triggerAction(QAK::QuickActionContext *actionContext, const QString &id, QObject *source = nullptr);
        static QObject *createActionObject(QAK::QuickActionContext *actionContext, const QString &id, bool shouldBeQuickAction = true);
        static inline QString removeMnemonic(const QString &s) {
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
    };

}

#endif // DIFFSCOPE_UTILS_ACTIONHELPER_H
