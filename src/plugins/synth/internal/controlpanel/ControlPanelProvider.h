// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_CONTROLPANELPROVIDER_H
#define DIFFSCOPE_SYNTH_CONTROLPANELPROVIDER_H

#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlParserStatus>
#include <QUrl>
#include <qqmlintegration.h>

namespace Synth::Internal {

    class ControlPanelProvider : public QObject, public QQmlParserStatus {
        Q_OBJECT
        QML_ELEMENT
        Q_INTERFACES(QQmlParserStatus)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
        Q_PROPERTY(bool preview READ preview WRITE setPreview NOTIFY previewChanged)
        Q_PROPERTY(QQmlComponent *architectureControlPanelComponent READ architectureControlPanelComponent NOTIFY panelsChanged)
        Q_PROPERTY(QQmlComponent *singerControlPanelComponent READ singerControlPanelComponent NOTIFY panelsChanged)
        Q_PROPERTY(bool loading READ loading NOTIFY panelsChanged)
        Q_PROPERTY(bool failed READ failed NOTIFY panelsChanged)

    public:
        explicit ControlPanelProvider(QObject *parent = nullptr);
        ~ControlPanelProvider() override;

        void classBegin() override;
        void componentComplete() override;

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);
        QUrl source() const;
        void setSource(const QUrl &source);
        bool preview() const;
        void setPreview(bool preview);
        QQmlComponent *architectureControlPanelComponent() const;
        QQmlComponent *singerControlPanelComponent() const;
        bool loading() const;
        bool failed() const;

    Q_SIGNALS:
        void architectureIdChanged();
        void sourceChanged();
        void previewChanged();
        void panelsChanged();

    private:
        void scheduleReload();
        void reload();
        void finishLoading();

        QString m_architectureId;
        QUrl m_source;
        bool m_preview{};
        bool m_complete{};
        bool m_reloadPending{};
        bool m_loading{};
        bool m_failed{};
        QQmlComponent *m_providerComponent{};
        QObject *m_provider{};
        QPointer<QQmlComponent> m_architecturePanel;
        QPointer<QQmlComponent> m_singerPanel;
    };

}

#endif // DIFFSCOPE_SYNTH_CONTROLPANELPROVIDER_H
