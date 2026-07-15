#ifndef DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_H
#define DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_H

#include <QObject>
#include <QStringList>

#include <coreplugin/coreglobal.h>
#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/SingerInfo.h>

namespace Core {

    class SingerRegistryPrivate;

    class CORE_EXPORT SingerRegistry : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SingerRegistry)
        Q_PROPERTY(QStringList architectureIds READ architectureIds NOTIFY architectureIdsChanged)

    public:
        explicit SingerRegistry(QObject *parent = nullptr);
        ~SingerRegistry() override;

        bool registerArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        bool updateArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        bool removeArchitecture(const QString &architectureId);
        bool containsArchitecture(const QString &architectureId) const;
        ArchitectureInfo architectureInfo(const QString &architectureId) const;
        QStringList architectureIds() const;

        bool registerSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        bool updateSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        bool removeSinger(const QString &architectureId, const QString &singerId);
        bool containsSinger(const QString &architectureId, const QString &singerId) const;
        SingerInfo singerInfo(const QString &architectureId, const QString &singerId) const;
        QStringList singerIds(const QString &architectureId) const;

    Q_SIGNALS:
        void architectureIdsChanged(const QStringList &architectureIds);
        void architectureRegistered(const QString &architectureId);
        void architectureUpdated(const QString &architectureId);
        void architectureRemoved(const QString &architectureId);
        void singerRegistered(const QString &architectureId, const QString &singerId);
        void singerUpdated(const QString &architectureId, const QString &singerId);
        void singerRemoved(const QString &architectureId, const QString &singerId);

    private:
        QScopedPointer<SingerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SINGERREGISTRY_H
