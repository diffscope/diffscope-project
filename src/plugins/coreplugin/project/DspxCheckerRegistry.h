#ifndef DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H
#define DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H

#include <QObject>

#include <coreplugin/IDspxChecker.h>

namespace QDspx {
    struct Model;
}

namespace Core {

    class DspxCheckerRegistryPrivate;

    class DspxCheckerRegistry : public IDspxChecker {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DspxCheckerRegistry)
    public:
        explicit DspxCheckerRegistry(QObject *parent = nullptr);
        ~DspxCheckerRegistry() override;

        void registerChecker(IDspxChecker *checker);
        QList<IDspxChecker *> checkers() const;

        QList<DspxCheckWarning> runCheck(const QDspx::Model &model, Level level, bool failFast) override;

    private:
        QScopedPointer<DspxCheckerRegistryPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H
