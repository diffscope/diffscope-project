// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_H
#define DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

#include <singercatalog/singercatalogglobal.h>

Q_MOC_INCLUDE(<dspxmodelORM/Sources.h>)

namespace dspx {
    class Sources;
}

namespace SingerCatalog {

    class ClipSingerIdProviderPrivate;

    /** @brief Tracks the architecture ID and recursive singer ID tree of document sources. */
    class SINGERCATALOG_EXPORT ClipSingerIdProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipSingerIdProvider)
        Q_PROPERTY(dspx::Sources *sources READ sources WRITE setSources NOTIFY sourcesChanged)
        Q_PROPERTY(QString architectureId READ architectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QVariantList singerTree READ singerTree NOTIFY singerTreeChanged)

    public:
        /** @brief Constructs an unbound provider with an optional QObject @p parent. */
        explicit ClipSingerIdProvider(QObject *parent = nullptr);
        /** @brief Destroys the provider. */
        ~ClipSingerIdProvider() override;

        /** @brief Returns the borrowed sources, or nullptr if unbound or the sources were destroyed. */
        dspx::Sources *sources() const;
        /** @brief Binds to @p sources without transferring ownership, or clears the binding if nullptr. */
        void setSources(dspx::Sources *sources);

        /** @brief Returns the sources category as the architecture ID, or an empty string if unbound. */
        QString architectureId() const;

        /**
         * @brief Returns the hierarchical singer ID tree, or an empty list if unbound.
         *
         * Each entry is a QString singer ID for a single singer or a recursive QVariantList for a mixed singer.
         * Item order matches each corresponding dspx::SingerList. Empty singer IDs and mixed lists are preserved.
         */
        QVariantList singerTree() const;

    Q_SIGNALS:
        /** @brief Signals a sources binding change, including sources destruction. */
        void sourcesChanged(dspx::Sources *sources);
        /** @brief Signals a change to the resolved architecture ID. */
        void architectureIdChanged(const QString &architectureId);
        /** @brief Signals a change to the resolved singer ID tree. */
        void singerTreeChanged(const QVariantList &singerTree);

    private:
        QScopedPointer<ClipSingerIdProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_CLIPSINGERIDPROVIDER_H
