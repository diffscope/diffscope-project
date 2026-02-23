#ifndef DIFFSCOPE_COREPLUGIN_DEFAULTLYRICMANAGER_H
#define DIFFSCOPE_COREPLUGIN_DEFAULTLYRICMANAGER_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class SingingClip;
}

namespace Core {

    class DefaultLyricManagerPrivate;

    class CORE_EXPORT DefaultLyricManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DefaultLyricManager)

    public:
        explicit DefaultLyricManager(QObject *parent = nullptr);
        ~DefaultLyricManager() override;

        Q_INVOKABLE QString getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const;

    private:
        QScopedPointer<DefaultLyricManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_DEFAULTLYRICMANAGER_H
