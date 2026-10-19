// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H

#include <QPointer>

#include <CoreApi/windowinterface.h>

class QSortFilterProxyModel;

namespace dspx {
    class SingingClip;
    class Track;
}

namespace VisualEditor {
    class PianoRollPanelInterface;
}

namespace VisualEditor::Internal {

    class AdditionalTrackLoader;

    class PianoRollAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::PianoRollPanelInterface *pianoRollPanelInterface READ pianoRollPanelInterface CONSTANT)
        Q_PROPERTY(AdditionalTrackLoader *additionalTrackLoader READ additionalTrackLoader CONSTANT)
        Q_PROPERTY(AdditionalTrackLoader *bottomAdditionalTrackLoader READ bottomAdditionalTrackLoader CONSTANT)
        Q_PROPERTY(bool altPressed READ altPressed NOTIFY altPressedChanged)
        Q_PROPERTY(bool trackSelectorVisible READ isTrackSelectorVisible WRITE setTrackSelectorVisible NOTIFY trackSelectorVisibleChanged)
        Q_PROPERTY(bool canEditPreviousTrack READ canEditPreviousTrack NOTIFY trackNavigationChanged)
        Q_PROPERTY(bool canEditNextTrack READ canEditNextTrack NOTIFY trackNavigationChanged)
        Q_PROPERTY(bool canEditPreviousClip READ canEditPreviousClip NOTIFY clipNavigationChanged)
        Q_PROPERTY(bool canEditNextClip READ canEditNextClip NOTIFY clipNavigationChanged)
        Q_PROPERTY(int editingClipIndex READ editingClipIndex NOTIFY clipNavigationChanged)
    public:
        explicit PianoRollAddOn(QObject *parent = nullptr);
        ~PianoRollAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        PianoRollPanelInterface *pianoRollPanelInterface() const;
        AdditionalTrackLoader *additionalTrackLoader() const;
        AdditionalTrackLoader *bottomAdditionalTrackLoader() const;

        bool canEditPreviousTrack() const;
        bool canEditNextTrack() const;
        bool canEditPreviousClip() const;
        bool canEditNextClip() const;
        int editingClipIndex() const;

        Q_INVOKABLE void editAdjacentTrack(bool next);
        Q_INVOKABLE void editAdjacentClip(bool next);
        Q_INVOKABLE void editTrack(dspx::Track *track);
        Q_INVOKABLE void editClip(dspx::SingingClip *clip);

        bool eventFilter(QObject *watched, QEvent *event) override;

        bool altPressed() const;

        bool isTrackSelectorVisible() const;
        void setTrackSelectorVisible(bool visible);

    Q_SIGNALS:
        void altPressedChanged();
        void trackSelectorVisibleChanged();
        void trackNavigationChanged();
        void clipNavigationChanged();

    private:
        void bindEditingNavigation();
        void updateTrackNavigation();
        void updateClipNavigation();
        void resetTimeViewStart();
        void adjustViewForEditingClip();

        AdditionalTrackLoader *m_additionalTrackLoader{};
        AdditionalTrackLoader *m_bottomAdditionalTrackLoader{};
        QSortFilterProxyModel *m_editingClipSelectorModel{};
        QPointer<dspx::Track> m_previousEditingTrack;
        QPointer<dspx::Track> m_nextEditingTrack;
        int m_editingClipIndex{-1};
        bool m_altPressed{};
        bool m_trackSelectorVisible{};
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
