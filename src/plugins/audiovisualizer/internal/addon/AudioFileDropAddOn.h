// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOVISUALIZER_AUDIOFILEDROPADDON_H
#define DIFFSCOPE_AUDIOVISUALIZER_AUDIOFILEDROPADDON_H

#include <QList>
#include <QPointer>
#include <QUrl>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core {
    class DspxDocument;
}

namespace dspx {
    class Track;
}

namespace AudioVisualizer::Internal {

    class AudioFileDropAddOn;

    class AudioFileDropAddOnAttachedType : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_PROPERTY(AudioFileDropAddOn *addOn READ addOn NOTIFY addOnChanged)
    public:
        explicit AudioFileDropAddOnAttachedType(QObject *parent);
        AudioFileDropAddOn *addOn() const;

    Q_SIGNALS:
        void addOnChanged();
    };

    class AudioFileDropAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        QML_ATTACHED(AudioFileDropAddOnAttachedType)
        Q_PROPERTY(bool importing READ isImporting NOTIFY importingChanged)
    public:
        explicit AudioFileDropAddOn(QObject *parent = nullptr);

        void initialize() override;
        void extensionsInitialized() override;
        static AudioFileDropAddOnAttachedType *qmlAttachedProperties(QObject *object);

        bool isImporting() const;
        Q_INVOKABLE bool acceptsUrls(const QList<QUrl> &urls) const;
        Q_INVOKABLE bool importFiles(const QList<QUrl> &urls, dspx::Track *firstTrack, int position);

    Q_SIGNALS:
        void importingChanged();

    private:
        void importFilesNow(const QList<QUrl> &urls, QPointer<Core::DspxDocument> document, QPointer<dspx::Track> firstTrack, bool hadFirstTrack, int position);

        bool m_importing{};
    };

}

#endif // DIFFSCOPE_AUDIOVISUALIZER_AUDIOFILEDROPADDON_H
