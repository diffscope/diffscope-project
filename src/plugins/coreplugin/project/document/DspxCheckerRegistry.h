#ifndef DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H
#define DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H

#include <QObject>

#include <coreplugin/IDspxChecker.h>

namespace opendspx {
    struct Model;
}

namespace Core {

    class DspxCheckerRegistryPrivate;

    class CORE_EXPORT DspxCheckerRegistry : public IDspxChecker {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DspxCheckerRegistry)
    public:
        explicit DspxCheckerRegistry(QObject *parent = nullptr);
        ~DspxCheckerRegistry() override;

        void registerChecker(IDspxChecker *checker);
        QList<IDspxChecker *> checkers() const;

        QList<DspxCheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) override;

    private:
        QScopedPointer<DspxCheckerRegistryPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCHECKERREGISTRY_H
