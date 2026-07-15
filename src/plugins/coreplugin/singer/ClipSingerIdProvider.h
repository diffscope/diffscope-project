#ifndef DIFFSCOPE_COREPLUGIN_CLIPSINGERIDPROVIDER_H
#define DIFFSCOPE_COREPLUGIN_CLIPSINGERIDPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class Sources;
}

namespace Core {

    class ClipSingerIdProviderPrivate;

    class CORE_EXPORT ClipSingerIdProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipSingerIdProvider)
        Q_PROPERTY(dspx::Sources *sources READ sources WRITE setSources NOTIFY sourcesChanged)
        Q_PROPERTY(QString architectureId READ architectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QVariantList singerTree READ singerTree NOTIFY singerTreeChanged)

    public:
        explicit ClipSingerIdProvider(QObject *parent = nullptr);
        ~ClipSingerIdProvider() override;

        dspx::Sources *sources() const;
        void setSources(dspx::Sources *sources);

        QString architectureId() const;

        /**
         * @brief Hierarchical singer ID tree extracted from sources.
         *
         * Each item has one of the following recursive forms:
         * - A QString for a single singer, containing its singer ID.
         * - A QVariantList for a mixed singer, containing the same forms for its child singers.
         *
         * Item order is identical to the order in the corresponding dspx::SingerList.
         */
        QVariantList singerTree() const;

    Q_SIGNALS:
        void sourcesChanged(dspx::Sources *sources);
        void architectureIdChanged(const QString &architectureId);
        void singerTreeChanged(const QVariantList &singerTree);

    private:
        QScopedPointer<ClipSingerIdProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_CLIPSINGERIDPROVIDER_H
