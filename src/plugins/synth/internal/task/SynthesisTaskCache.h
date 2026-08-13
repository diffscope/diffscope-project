#ifndef DIFFSCOPE_SYNTH_SYNTHESISTASKCACHE_H
#define DIFFSCOPE_SYNTH_SYNTHESISTASKCACHE_H

#include <QByteArray>
#include <QString>
#include <QTemporaryDir>

#include <synth/SynthesisModel.h>

namespace Synth::Internal {

    class SynthesisTaskCache {
    public:
        SynthesisTaskCache();

        void reload();

        bool read(const QByteArray &key, SynthesisTaskResult *result) const;
        bool write(const QByteArray &key, const SynthesisTaskResult &result);
        QString audioPath(const QByteArray &key, const QString &suffix, bool persistent) const;
        bool writeBytes(const QString &path, const QByteArray &bytes) const;

        qint64 size() const;
        void clear();
        void trim() const;

        qint64 maximumDownloadBytes() const;
        int environmentTagTtlSeconds() const;

    private:
        QString entryPath(const QByteArray &key) const;

        QString m_root;
        qint64 m_maximumBytes{};
        int m_expiryDays{};
        qint64 m_maximumDownloadBytes{};
        int m_environmentTagTtlSeconds{};
        mutable QTemporaryDir m_temporaryDir;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISTASKCACHE_H
