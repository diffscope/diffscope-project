#ifndef DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H

#include <QPointer>
#include <QSet>

#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class NoteSequence;
    class SingingClip;
    class Track;

    class NoteSelectionModelPrivate {
        Q_DECLARE_PUBLIC(NoteSelectionModel)
    public:
        NoteSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Note *> selectedItems;
        Note *currentItem = nullptr;
        NoteSequence *noteSequenceWithSelectedItems = nullptr;
        QSet<Note *> connectedItems;
        QPointer<SingingClip> connectedSingingClip;
        QPointer<Track> connectedTrack;
        QMetaObject::Connection noteSequenceDestroyedConnection;
        QMetaObject::Connection singingClipConnection;
        QMetaObject::Connection trackListConnection;

        bool isValidItem(Note *item) const;
        void connectItem(Note *item);
        void disconnectItem(Note *item);
        bool addToSelection(Note *item);
        bool removeFromSelection(Note *item);
        bool clearSelection();
        void dropItem(Note *item);
        void setCurrentItem(Note *item);

        void connectNoteSequence(NoteSequence *noteSeq);
        void disconnectNoteSequence();
        void clearAllAndResetNoteSequence();
        void connectTrackForNoteSequence(const QPointer<NoteSequence> &noteSeq);

        void select(Note *item, SelectionModel::SelectionCommand command, NoteSequence *containerItemHint);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H