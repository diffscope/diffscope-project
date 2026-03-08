#include "DspxCheckerRegistry.h"
#include "DspxCheckerRegistry_p.h"


namespace Core {

    DspxCheckerRegistry::DspxCheckerRegistry(QObject *parent) : IDspxChecker(parent), d_ptr(new DspxCheckerRegistryPrivate) {
        Q_D(DspxCheckerRegistry);
        d->q_ptr = this;
    }

    DspxCheckerRegistry::~DspxCheckerRegistry() = default;

    void DspxCheckerRegistry::registerChecker(IDspxChecker *checker) {
        Q_D(DspxCheckerRegistry);
        d->checkers.append(checker);
    }

    QList<IDspxChecker *> DspxCheckerRegistry::checkers() const {
        Q_D(const DspxCheckerRegistry);
        return d->checkers;
    }

    QList<DspxCheckWarning> DspxCheckerRegistry::runCheck(const QDspx::Model &model, Level level, bool failFast) {
        Q_D(DspxCheckerRegistry);
        for (auto checker : d->checkers) {
            auto warnings = checker->runCheck(model, level, failFast);
            if (failFast && !warnings.isEmpty())
                return warnings;
        }
        return {};
    }

}
