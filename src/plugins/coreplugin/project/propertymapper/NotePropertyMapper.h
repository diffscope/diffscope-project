#ifndef DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_H
#define DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_H

#include <QObject>
#include <QVariant>
#include <QScopedPointer>
#include <qqmlintegration.h>

namespace dspx {
    class SelectionModel;
}

namespace Core {
    class NotePropertyMapperPrivate;

    class NotePropertyMapper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(NotePropertyMapper)

        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel WRITE setSelectionModel NOTIFY selectionModelChanged)
        Q_PROPERTY(QVariant centShift READ centShift WRITE setCentShift NOTIFY centShiftChanged)
        Q_PROPERTY(QVariant keyNum READ keyNum WRITE setKeyNum NOTIFY keyNumChanged)
        Q_PROPERTY(QVariant language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(QVariant length READ length WRITE setLength NOTIFY lengthChanged)
        Q_PROPERTY(QVariant lyric READ lyric WRITE setLyric NOTIFY lyricChanged)
        Q_PROPERTY(QVariant pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QVariant pronunciationOriginal READ pronunciationOriginal WRITE setPronunciationOriginal NOTIFY pronunciationOriginalChanged)
        Q_PROPERTY(QVariant pronunciationEdited READ pronunciationEdited WRITE setPronunciationEdited NOTIFY pronunciationEditedChanged)
        Q_PROPERTY(QVariant singingClip READ singingClip NOTIFY singingClipChanged)

    public:
        explicit NotePropertyMapper(QObject *parent = nullptr);
        ~NotePropertyMapper() override;

        dspx::SelectionModel *selectionModel() const;
        void setSelectionModel(dspx::SelectionModel *selectionModel);

        QVariant centShift() const;
        void setCentShift(const QVariant &centShift);

        QVariant keyNum() const;
        void setKeyNum(const QVariant &keyNum);

        QVariant language() const;
        void setLanguage(const QVariant &language);

        QVariant length() const;
        void setLength(const QVariant &length);

        QVariant lyric() const;
        void setLyric(const QVariant &lyric);

        QVariant pos() const;
        void setPos(const QVariant &pos);

        QVariant pronunciationOriginal() const;
        void setPronunciationOriginal(const QVariant &pronunciationOriginal);

        QVariant pronunciationEdited() const;
        void setPronunciationEdited(const QVariant &pronunciationEdited);

        QVariant singingClip() const;

    Q_SIGNALS:
        void selectionModelChanged();
        void centShiftChanged();
        void keyNumChanged();
        void languageChanged();
        void lengthChanged();
        void lyricChanged();
        void posChanged();
        void pronunciationOriginalChanged();
        void pronunciationEditedChanged();
        void singingClipChanged();

    private:
        QScopedPointer<NotePropertyMapperPrivate> d_ptr;
    };
}

#endif // DIFFSCOPE_COREPLUGIN_NOTEPROPERTYMAPPER_H