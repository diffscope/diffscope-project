// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H
#define DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H

#include <QObject>

#include <dspxproject/IChecker.h>

namespace opendspx {
    struct Model;
}

namespace DspxProject {

    class CheckerRegistryPrivate;

    /** Runs borrowed checkers in registration order. */
    class DSPXPROJECT_EXPORT CheckerRegistry : public IChecker {
        Q_OBJECT
        Q_DECLARE_PRIVATE(CheckerRegistry)
    public:
        /** Creates an empty checker registry. */
        explicit CheckerRegistry(QObject *parent = nullptr);
        /** Destroys the registry without deleting registered checkers. */
        ~CheckerRegistry() override;

        /**
         * Appends a checker without taking ownership.
         * @pre checker is non-null and remains valid for the registry's lifetime.
         */
        void registerChecker(IChecker *checker);
        /** Returns the borrowed checkers in registration order. */
        QList<IChecker *> checkers() const;

        /**
         * Runs registered checkers at the requested level.
         * Returns the first nonempty warning list if failFast is true.
         * Returns an empty list after all checkers complete otherwise.
         */
        QList<CheckWarning> runCheck(const opendspx::Model &model, Level level, bool failFast) override;

    private:
        QScopedPointer<CheckerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CHECKERREGISTRY_H
