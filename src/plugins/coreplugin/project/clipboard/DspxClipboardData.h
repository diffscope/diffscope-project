#ifndef DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARDDATA_H
#define DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARDDATA_H

#include <variant>

#include <QList>

#include <opendspx/model.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    struct CORE_EXPORT DspxClipboardData {

        enum Type {
            Tempo,
            Label,
            Track,
            Clip,
            Note,
            // TODO param
        };

        Type type() const {
            return static_cast<Type>(m_data.index());
        }

        void setTempos(const QList<QDspx::Tempo> &tempos) {
            m_data = tempos;
        }

        QList<QDspx::Tempo> tempos() const {
            return std::get<Tempo>(m_data);
        }

        void setLabels(const QList<QDspx::Label> &labels) {
            m_data = labels;
        }

        QList<QDspx::Label> labels() const {
            return std::get<Label>(m_data);
        }

        void setTracks(const QList<QDspx::Track> &tracks) {
            m_data = tracks;
        }

        QList<QDspx::Track> tracks() const {
            return std::get<Track>(m_data);
        }

        void setClips(const QList<QList<QDspx::ClipRef>> &clips) {
            m_data = clips;
        }

        QList<QList<QDspx::ClipRef>> clips() const {
            return std::get<Clip>(m_data);
        }

        void setNotes(const QList<QDspx::Note> &notes) {
            m_data = notes;
        }

        QList<QDspx::Note> notes() const {
            return std::get<Note>(m_data);
        }

        int playhead() const {
            return m_playhead;
        }

        void setPlayhead(int playhead) {
            m_playhead = playhead;
        }

        int absolute() const {
            return m_absolute;
        }

        void setAbsolute(int absolute) {
            m_absolute = absolute;
        }

        int track() const {
            return m_track;
        }

        void setTrack(int track) {
            m_track = track;
        }

        static QString mimeType(Type type);
        static Type typeFromMimeType(const QString &mimeType, bool *ok = nullptr);
        QByteArray toData() const;
        static DspxClipboardData fromData(const QByteArray &data, Type type, bool *ok = nullptr);

    private:
        std::variant<
            QList<QDspx::Tempo>,
            QList<QDspx::Label>,
            QList<QDspx::Track>,
            QList<QList<QDspx::ClipRef>>,
            QList<QDspx::Note>
        > m_data;
        int m_playhead{};
        int m_absolute{};
        int m_track{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARDDATA_H
