#ifndef DIFFSCOPE_DSPX_MODEL_TRACKLIST_H
#define DIFFSCOPE_DSPX_MODEL_TRACKLIST_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Track;
}

namespace dspx {

    class Track;

    class TrackListPrivate;

    class DSPX_MODEL_EXPORT TrackList : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TrackList)
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QList<Track *> items READ items NOTIFY itemsChanged)
        Q_PRIVATE_PROPERTY(d_func(), QJSValue iterable READ iterable CONSTANT)

    public:
        ~TrackList() override;

        int size() const;
        QList<Track *> items() const;
        Q_INVOKABLE bool insertItem(int index, Track *item);
        Q_INVOKABLE Track *removeItem(int index);
        Q_INVOKABLE Track *item(int index) const;
        Q_INVOKABLE bool rotate(int leftIndex, int middleIndex, int rightIndex);

        QList<QDspx::Track> toQDspx() const;
        void fromQDspx(const QList<QDspx::Track> &trackList);

    Q_SIGNALS:
        void itemAboutToInsert(int index, Track *item);
        void itemInserted(int index, Track *item);
        void itemAboutToRemove(int index, Track *item);
        void itemRemoved(int index, Track *item);
        void aboutToRotate(int leftIndex, int middleIndex, int rightIndex);
        void rotated(int leftIndex, int middleIndex, int rightIndex);
        void sizeChanged(int size);
        void itemsChanged();

    protected:
        void handleInsertIntoListContainer(Handle entity, int index) override;
        void handleTakeFromListContainer(Handle takenEntity, int index) override;
        void handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) override;

    private:
        friend class ModelPrivate;
        explicit TrackList(Handle handle, Model *model);
        QScopedPointer<TrackListPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKLIST_H