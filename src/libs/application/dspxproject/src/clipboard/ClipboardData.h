// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARDDATA_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARDDATA_H

#include <variant>
#include <vector>

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVariant>

#include <stdcorelib/support/json.h>

#include <opendspx/model.h>

#include <dspxproject/dspxprojectglobal.h>

namespace DspxProject {

    /**
     * @brief Sampled parameter data relative to the copied interval.
     *
     * The interval is [0, length) in ticks. Sample i is at offset + i * 5 ticks.
     * Boundary samples outside the interval preserve the source sampling grid for
     * interpolation. Invalid variants represent gaps. Missing samples also represent
     * gaps. Valid variants contain integer model values.
     *
     * @pre length is positive and does not exceed INT_MAX - 10.
     * @pre offset is greater than -5 and no greater than 0.
     * @pre values contains at most ceil((length - offset) / 5) + 1 samples.
     */
    struct DSPXPROJECT_EXPORT FreeParameterClipboardData {
        /** The copied interval length in ticks. */
        int length{};
        /** The first sample position relative to the interval start in ticks. */
        int offset{};
        /** The source samples, including available boundary neighbors. */
        QList<QVariant> values;
    };

    /**
     * @brief A typed clipboard payload and its placement metadata.
     *
     * Default construction produces an empty Tempo payload with zero metadata.
     * Payload setters replace the type and preserve placement metadata. Each getter
     * requires its corresponding type. Clip references and singer references in
     * payloads, including nested references, must be non-null.
     *
     * @par MIME types
     * Each type uses the application/x.diffscope.clipboard. prefix and +json suffix.
     * The intervening names are
     * tempo, label, keysignature, track, clip, note, dynamicmixinganchor, anchornode,
     * and freeparameter, respectively. The MIME type supplies the payload type.
     * The JSON object has no separate type field.
     *
     * @par JSON envelope
     * The representation is a UTF-8 JSON object with version, playhead, absolute,
     * track, and data fields. version is the DSPX serialization version string.
     * playhead, absolute, and track are integers. data is an array. Item encoding
     * uses the DSPX version identified by version, except for the clipboard-specific
     * structures described below. toData() uses DSPX V1 serialization. Metadata
     * omitted during decoding defaults to zero.
     *
     * @par Placement
     * For sequence payloads, positions are relative to the earliest copied item.
     * absolute records that item's original position. playhead records the copied
     * playhead position minus absolute in the same coordinate system. Tempo, label,
     * and clip positions use project ticks. Note, anchor node, dynamic mixing anchor,
     * and free parameter positions use ticks relative to the singing clip's content
     * origin. Track payloads retain their contained positions and use zero metadata.
     *
     * @par Destination context
     * Note, dynamic mixing anchor, and parameter payloads contain no destination clip
     * identity. Parameter payloads contain no parameter identifier or transform or
     * edited role. The active edit scope supplies these destinations. Dynamic mixing
     * anchors contain no singer definitions. Ratios use the destination singer order.
     *
     * @par Clip grouping
     * Clip data contains one array per source track from the first copied track
     * through the last copied track. Empty arrays preserve intervening track gaps.
     * All clip positions share the earliest copied clip as their origin. track is
     * the zero-based source index of the first track. Other payload types ignore track.
     *
     * @par Key signatures
     * Each key signature is an object with pos, mode, tonality, and accidentalType.
     * pos is the relative tick position. mode is the 12-bit scale mask. tonality is
     * the pitch class from 0 through 11. accidentalType is 0 for flats or 1 for sharps.
     *
     * @par Free parameters
     * data contains exactly one object with integer length and offset fields and a
     * values array, as specified by FreeParameterClipboardData. Each values entry
     * is an integer sample or JSON null for a gap. absolute is the original interval
     * start. playhead is relative to that start.
     */
    struct DSPXPROJECT_EXPORT ClipboardData {

        /** The payload types and their corresponding MIME representations. */
        enum Type {
            Tempo, /**< A tempo sequence. */
            Label, /**< A label sequence. */
            KeySignature, /**< A key signature sequence. */
            Track, /**< An ordered track list. */
            Clip, /**< Clip arrays grouped by source track. */
            Note, /**< A note sequence. */
            DynamicMixingAnchor, /**< A dynamic mixing anchor sequence. */
            AnchorNode, /**< A parameter anchor node sequence. */
            FreeParameter, /**< A sampled parameter interval. */
        };

        /** Returns the active payload type. */
        Type type() const {
            return static_cast<Type>(m_data.index());
        }

        /** Replaces the payload with tempos. */
        void setTempos(const QList<opendspx::Tempo> &tempos) {
            m_data = tempos;
        }

        /** Returns the tempos. @pre type() is Tempo. */
        QList<opendspx::Tempo> tempos() const {
            return std::get<Tempo>(m_data);
        }

        /** Replaces the payload with labels. */
        void setLabels(const QList<opendspx::Label> &labels) {
            m_data = labels;
        }

        /** Returns the labels. @pre type() is Label. */
        QList<opendspx::Label> labels() const {
            return std::get<Label>(m_data);
        }

        /** Replaces the payload with clipboard key signature objects. */
        void setKeySignatures(const QList<stdc::json::Value> &keySignatures) {
            m_data = keySignatures;
        }

        /** Returns the key signatures. @pre type() is KeySignature. */
        QList<stdc::json::Value> keySignatures() const {
            return std::get<KeySignature>(m_data);
        }

        /** Replaces the payload with ordered tracks. */
        void setTracks(const QList<opendspx::Track> &tracks) {
            m_data = tracks;
        }

        /** Returns the tracks. @pre type() is Track. */
        QList<opendspx::Track> tracks() const {
            return std::get<Track>(m_data);
        }

        /** Replaces the payload with clip arrays grouped by source track. */
        void setClips(const QList<std::vector<opendspx::ClipRef>> &clips) {
            m_data = clips;
        }

        /** Returns the clip groups. @pre type() is Clip. */
        QList<std::vector<opendspx::ClipRef>> clips() const {
            return std::get<Clip>(m_data);
        }

        /** Replaces the payload with notes. */
        void setNotes(const QList<opendspx::Note> &notes) {
            m_data = notes;
        }

        /** Returns the notes. @pre type() is Note. */
        QList<opendspx::Note> notes() const {
            return std::get<Note>(m_data);
        }

        /** Replaces the payload with dynamic mixing anchors. */
        void setDynamicMixingAnchors(const QList<opendspx::DynamicMixingAnchor> &anchors) {
            m_data = anchors;
        }

        /** Returns the dynamic mixing anchors. @pre type() is DynamicMixingAnchor. */
        QList<opendspx::DynamicMixingAnchor> dynamicMixingAnchors() const {
            return std::get<DynamicMixingAnchor>(m_data);
        }

        /** Replaces the payload with parameter anchor nodes. */
        void setAnchorNodes(const QList<opendspx::AnchorNode> &nodes) {
            m_data = nodes;
        }

        /** Returns the anchor nodes. @pre type() is AnchorNode. */
        QList<opendspx::AnchorNode> anchorNodes() const {
            return std::get<AnchorNode>(m_data);
        }

        /** Replaces the payload with a sampled parameter interval. */
        void setFreeParameter(const FreeParameterClipboardData &parameter) {
            m_data = parameter;
        }

        /** Returns the sampled interval. @pre type() is FreeParameter. */
        FreeParameterClipboardData freeParameter() const {
            return std::get<FreeParameter>(m_data);
        }

        /** Returns the copied playhead offset from the payload origin in ticks. */
        int playhead() const {
            return m_playhead;
        }

        /** Sets the copied playhead offset from the payload origin in ticks. */
        void setPlayhead(int playhead) {
            m_playhead = playhead;
        }

        /** Returns the original payload origin in project or clip content ticks. */
        int absolute() const {
            return m_absolute;
        }

        /** Sets the original payload origin in project or clip content ticks. */
        void setAbsolute(int absolute) {
            m_absolute = absolute;
        }

        /** Returns the first source track index for clip payloads. */
        int track() const {
            return m_track;
        }

        /** Sets the zero-based first source track index for clip payloads. */
        void setTrack(int track) {
            m_track = track;
        }

        /** Returns the MIME type, or an empty string for an unknown type. */
        static QString mimeType(Type type);
        /**
         * Returns the matching type, or Tempo for an unrecognized MIME type.
         * Writes recognition success to ok if non-null.
         */
        static Type typeFromMimeType(const QString &mimeType, bool *ok = nullptr);
        /** Serializes the payload and placement metadata to compact JSON. */
        QByteArray toData() const;
        /**
         * Decodes JSON using the type supplied by its MIME representation.
         * Returns default data on parse, version, or payload errors. Writes decoding
         * success to ok if non-null. The result is usable only on success.
         */
        static ClipboardData fromData(const QByteArray &data, Type type, bool *ok = nullptr);

    private:
        std::variant<
            QList<opendspx::Tempo>,
            QList<opendspx::Label>,
            QList<stdc::json::Value>,
            QList<opendspx::Track>,
            QList<std::vector<opendspx::ClipRef>>,
            QList<opendspx::Note>,
            QList<opendspx::DynamicMixingAnchor>,
            QList<opendspx::AnchorNode>,
            FreeParameterClipboardData
        > m_data;
        int m_playhead{};
        int m_absolute{};
        int m_track{};
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARDDATA_H
