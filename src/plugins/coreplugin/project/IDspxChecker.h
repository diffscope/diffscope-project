#ifndef DIFFSCOPE_COREPLUGIN_IDSPXCHECKER_H
#define DIFFSCOPE_COREPLUGIN_IDSPXCHECKER_H

#include <QObject>
#include <QVariant>

namespace QDspx {
    struct Model;
}

namespace Core {

    struct DspxCheckWarning;

    class IDspxChecker : public QObject {
        Q_OBJECT
    public:
        explicit IDspxChecker(QObject *parent = nullptr);
        ~IDspxChecker() override;

        enum Level {
            Weak,
            Strong,
        };

        virtual QList<DspxCheckWarning> runCheck(const QDspx::Model &model, Level level, bool failFast) = 0;
    };

    struct DspxCheckWarning {
        IDspxChecker::Level level;
        QString message;
        QString jsonPath;
        QString description;
        QList<QPair<QString, QVariant>> info;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_IDSPXCHECKER_H
