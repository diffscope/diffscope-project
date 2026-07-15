#ifndef DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_H
#define DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>
#include <coreplugin/SingerInfo.h>

namespace Core {

    class SingerRegistry;
    class SingerInfoProviderPrivate;

    class CORE_EXPORT SingerInfoProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(SingerInfoProvider)
        Q_PROPERTY(SingerRegistry *registry READ registry WRITE setRegistry NOTIFY registryChanged)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QString singerId READ singerId WRITE setSingerId NOTIFY singerIdChanged)
        Q_PROPERTY(SingerInfo info READ info NOTIFY infoChanged)
        Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)

    public:
        explicit SingerInfoProvider(QObject *parent = nullptr);
        ~SingerInfoProvider() override;

        SingerRegistry *registry() const;
        void setRegistry(SingerRegistry *registry);

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);

        QString singerId() const;
        void setSingerId(const QString &singerId);

        SingerInfo info() const;
        bool exists() const;

    Q_SIGNALS:
        void registryChanged(SingerRegistry *registry);
        void architectureIdChanged(const QString &architectureId);
        void singerIdChanged(const QString &singerId);
        void infoChanged(const SingerInfo &info);
        void existsChanged(bool exists);

    private:
        QScopedPointer<SingerInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_SINGERINFOPROVIDER_H
